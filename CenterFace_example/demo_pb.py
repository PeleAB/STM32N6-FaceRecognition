import time
import argparse
import cv2 as cv2
import numpy as np
import tensorflow as tf


class CenterFace(object):
    def __init__(self, model_path:str):
        # Load TensorFlow model
        self.graph = tf.Graph()
        self.sess = tf.compat.v1.Session(graph=self.graph)
        
        with self.graph.as_default():
            # Load the graph definition
            with tf.io.gfile.GFile(model_path, 'rb') as f:
                graph_def = tf.compat.v1.GraphDef()
                graph_def.ParseFromString(f.read())
                tf.import_graph_def(graph_def, name='')
            
            # List all operations and their outputs
            ops = self.graph.get_operations()
            
            # Find input tensor (usually a placeholder)
            input_tensors = [op.outputs[0] for op in ops if op.type == 'Placeholder']
            if input_tensors:
                self.input_tensor = input_tensors[0]
            else:
                # Fallback: use first operation with no inputs
                for op in ops:
                    if len(op.inputs) == 0:
                        self.input_tensor = op.outputs[0]
                        break
            
            # Find output tensors (tensors with no consumers)
            output_tensors = []
            for op in ops:
                for output in op.outputs:
                    if not output.consumers():
                        output_tensors.append(output)
            
            # Need exactly 4 output tensors for CenterFace
            if len(output_tensors) != 4:
                print(f"Warning: Found {len(output_tensors)} output tensors, expected 4.")
                # Take last 4 tensors or operations if we have too many/few
                if len(output_tensors) > 4:
                    self.output_tensors = output_tensors[-4:]
                else:
                    # Get last operations as fallback
                    self.output_tensors = [ops[-4+i].outputs[0] for i in range(min(4, len(ops)))]
            else:
                self.output_tensors = output_tensors
            
            # Print selected tensors for verification
            print(f"Using input tensor: {self.input_tensor.name}")
            print("Using output tensors:")
            for tensor in self.output_tensors:
                print(f"  {tensor.name}")

    def transform(self, h, w):
        img_h_new , img_w_new = 32, 32 
        scale_h, scale_w = img_h_new / h, img_w_new / w
        return img_h_new, img_w_new, scale_h, scale_w

    def inference(self, img, threshold):
        h, w = img.shape[:2]
        img_h_new, img_w_new, scale_h, scale_w = self.transform(h, w)

        # Preprocess image for TensorFlow
        input_image = cv2.dnn.blobFromImage(img, scalefactor=1.0,
                                           size=(img_w_new, img_h_new),
                                           mean=(0, 0, 0), swapRB=True, crop=False)
        
        input_image = np.transpose(input_image, (0, 2, 3, 1))

        # Run inference with TensorFlow session
        outputs = self.sess.run(
            self.output_tensors, 
            feed_dict={self.input_tensor: input_image}
        )
        
        heatmap, scale, offset, lms = outputs
        return self.postprocess(heatmap, lms, offset, scale, scale_h, scale_w, img_h_new, img_w_new, threshold)

    def postprocess(self, heatmap, lms, offset, scale,
                    scale_h, scale_w, orig_h, orig_w, threshold):
        """
        Decode in 32×32 space, then scale boxes/landmarks up to the original frame.
        """
        # Pass the (32,32) size to decode, because that’s the domain the network used.
        dets, lms_pts = self.decode(
            heatmap, scale, offset, lms,
            (32, 32),  # decode in 32×32 domain
            threshold=threshold
        )
        # Now scale detections/landmarks back to the original size
        if len(dets) > 0:
            # dets[:, 0::2] => x1, x2 columns; dets[:, 1::2] => y1, y2
            dets[:, [0, 2]] /= scale_w  # x coords
            dets[:, [1, 3]] /= scale_h  # y coords

            # lms_pts shape: [N, 10] => 5 pairs of (x, y)
            lms_pts[:, 0::2] /= scale_w
            lms_pts[:, 1::2] /= scale_h

        return dets, lms_pts

    def decode(self, heatmap, scale, offset, landmark, size, threshold):
        """
        The TFLite outputs have shape:
          heatmap:  [1, 8, 8, 1]
          scale:    [1, 8, 8, 2]
          offset:   [1, 8, 8, 2]
          landmark: [1, 8, 8, 10]
        For 32×32 input, H/4 = 8, W/4 = 8.

        'size' here = (32, 32), so we clamp boxes to [0..32] in decode.
        We'll rescale them to the original frame afterwards in postprocess().
        """
        heatmap = heatmap[0]  # shape [8, 8, 1]
        scale   = scale[0]    # shape [8, 8, 2]
        offset  = offset[0]   # shape [8, 8, 2]
        landmark= landmark[0] # shape [8, 8, 10]

        heatmap = heatmap[..., 0]  # shape [8, 8]
        scale0  = scale[..., 0]
        scale1  = scale[..., 1]
        offset0 = offset[..., 0]
        offset1 = offset[..., 1]

        c0, c1 = np.where(heatmap > threshold)
        boxes, lms_list = [], []

        if len(c0) > 0:
            for i in range(len(c0)):
                y_idx = c0[i]
                x_idx = c1[i]

                s0 = np.exp(scale0[y_idx, x_idx]) * 4
                s1 = np.exp(scale1[y_idx, x_idx]) * 4
                o0 = offset0[y_idx, x_idx]
                o1 = offset1[y_idx, x_idx]
                score = heatmap[y_idx, x_idx]

                # Box corners in 32×32 domain
                x1 = max(0, (x_idx + o1 + 0.5) * 4 - s1 / 2)
                y1 = max(0, (y_idx + o0 + 0.5) * 4 - s0 / 2)
                x2 = min(x1 + s1, size[1])
                y2 = min(y1 + s0, size[0])

                boxes.append([x1, y1, x2, y2, score])

                # Landmarks in 32×32 domain
                # landmark[y_idx, x_idx] => 10 values => [y0, x0, y1, x1, ... y4, x4]
                lms_temp = []
                for j in range(5):
                    lm_y = landmark[y_idx, x_idx, j*2 + 0]
                    lm_x = landmark[y_idx, x_idx, j*2 + 1]
                    px = lm_x * s1 + x1
                    py = lm_y * s0 + y1
                    lms_temp.extend([px, py])

                lms_list.append(lms_temp)

            boxes = np.asarray(boxes, dtype=np.float32)
            lms_list = np.asarray(lms_list, dtype=np.float32)

            keep = self.nms(boxes[:, :4], boxes[:, 4], 0.1)
            boxes = boxes[keep, :]
            lms_list = lms_list[keep, :]

        return boxes, lms_list

    def nms(self, boxes, scores, nms_thresh):
        x1 = boxes[:, 0]
        y1 = boxes[:, 1]
        x2 = boxes[:, 2]
        y2 = boxes[:, 3]
        areas = (x2 - x1 + 1) * (y2 - y1 + 1)
        order = np.argsort(scores)[::-1]
        num_detections = boxes.shape[0]
        suppressed = np.zeros((num_detections,), dtype= bool)

        keep = []
        for _i in range(num_detections):
            i = order[_i]
            if suppressed[i]:
                continue
            keep.append(i)

            ix1 = x1[i]
            iy1 = y1[i]
            ix2 = x2[i]
            iy2 = y2[i]
            iarea = areas[i]

            for _j in range(_i + 1, num_detections):
                j = order[_j]
                if suppressed[j]:
                    continue

                xx1 = max(ix1, x1[j])
                yy1 = max(iy1, y1[j])
                xx2 = min(ix2, x2[j])
                yy2 = min(iy2, y2[j])
                w = max(0, xx2 - xx1 + 1)
                h = max(0, yy2 - yy1 + 1)

                inter = w * h
                ovr = inter / (iarea + areas[j] - inter)
                if ovr >= nms_thresh:
                    suppressed[j] = True

        return keep


def main():
    # Hardcoded parameters
    model_path = 'Models/model_float32.pb'  # works but not well
    threshold = 0.05

    centerface = CenterFace(model_path)

    # Use default webcam (index 0)
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        
        height, width = frame.shape[:2]

        # Find the minimum dimension
        min_dim = min(height, width)
        x_start = (width - min_dim) // 2
        y_start = (height - min_dim) // 2
        x_end = x_start + min_dim
        y_end = y_start + min_dim

        # Crop the square patch
        square_frame = frame[y_start:y_end, x_start:x_end]

        # Use the square frame for inference
        frame = square_frame
        
        t = time.time()
        dets, lms = centerface.inference(frame, threshold)
        dt = time.time() - t
        fps = 1/dt
        
        # Draw detections and landmarks
        for det in dets:
            boxes, score = det[:4], det[4]
            cv2.rectangle(frame, (int(boxes[0]), int(boxes[1])), (int(boxes[2]), int(boxes[3])), (2, 255, 0), 1)
            # Add confidence score text above the bounding box
            score_text = f"{score:.2f}"
            cv2.putText(frame, score_text, (int(boxes[0]), int(boxes[1])-5), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
        for lm in lms:
            for i in range(0, 5):
                cv2.circle(frame, (int(lm[i * 2]), int(lm[i * 2 + 1])), 2, (0, 0, 255), -1)
        
        # Add FPS text to frame
        cv2.putText(frame, f"FPS: {fps:.2f}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        cv2.imshow('centerface', frame)

        key = cv2.waitKey(1)
        if key == 27:  # ESC
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
