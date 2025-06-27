import time
import argparse
import cv2 as cv2
import numpy as np
import onnxruntime
import os


#my_frames = []

class CenterFace(object):
    def __init__(self, model_path:str):
        self.sess = onnxruntime.InferenceSession(
                        model_path,
                        providers=['CUDAExecutionProvider', 'CPUExecutionProvider'])

        self.input_name = self.sess.get_inputs()[0].name
        self.output_names = [out.name for out in self.sess.get_outputs()]

    def transform(self, h, w):
        #img_h_new, img_w_new = int(np.ceil(h / 32) * 32), int(np.ceil(w / 32) * 32)  #original code
        img_h_new , img_w_new = h, w #for test 
        scale_h, scale_w = img_h_new / h, img_w_new / w
        return img_h_new, img_w_new, scale_h, scale_w

    def inference(self, img, threshold):
        h, w = img.shape[:2]
        img_h_new, img_w_new, scale_h, scale_w = self.transform(h, w)

        input_image = cv2.dnn.blobFromImage(img, scalefactor=1.0,
                                            size=(img_w_new, img_h_new),
                                            mean=(0, 0, 0), swapRB=True, crop=False)
        #my_frames.append(input_image)
        outputs = self.sess.run(self.output_names, {self.input_name: input_image})
        heatmap, scale, offset, lms = outputs
        return self.postprocess(heatmap, lms, offset, scale, scale_h, scale_w, img_h_new, img_w_new, threshold)

    def postprocess(self, heatmap, lms, offset, scale, scale_h, scale_w, img_h_new, img_w_new, threshold):
        dets, lms = self.decode(heatmap, scale, offset, lms, (img_h_new, img_w_new), threshold=threshold)
        if len(dets) > 0:
            dets[:, 0:4:2], dets[:, 1:4:2] = dets[:, 0:4:2] / scale_w, dets[:, 1:4:2] / scale_h
            lms[:, 0:10:2], lms[:, 1:10:2] = lms[:, 0:10:2] / scale_w, lms[:, 1:10:2] / scale_h
        else:
            dets = np.empty(shape=[0, 5], dtype=np.float32)
            lms = np.empty(shape=[0, 10], dtype=np.float32)
        return dets, lms

    def decode(self, heatmap, scale, offset, landmark, size, threshold=0.1):
        heatmap = np.squeeze(heatmap)
        scale0, scale1 = scale[0, 0, :, :], scale[0, 1, :, :]
        offset0, offset1 = offset[0, 0, :, :], offset[0, 1, :, :]
        c0, c1 = np.where(heatmap > threshold)
        boxes, lms = [], []
        if len(c0) > 0:
            for i in range(len(c0)):
                s0, s1 = np.exp(scale0[c0[i], c1[i]]) * 4, np.exp(scale1[c0[i], c1[i]]) * 4
                o0, o1 = offset0[c0[i], c1[i]], offset1[c0[i], c1[i]]
                s = heatmap[c0[i], c1[i]]
                x1, y1 = max(0, (c1[i] + o1 + 0.5) * 4 - s1 / 2), max(0, (c0[i] + o0 + 0.5) * 4 - s0 / 2)
                x1, y1 = min(x1, size[1]), min(y1, size[0])
                boxes.append([x1, y1, min(x1 + s1, size[1]), min(y1 + s0, size[0]), s])
                lm = []
                for j in range(5):
                    lm.append(landmark[0, j * 2 + 1, c0[i], c1[i]] * s1 + x1)
                    lm.append(landmark[0, j * 2, c0[i], c1[i]] * s0 + y1)
                lms.append(lm)
            boxes = np.asarray(boxes, dtype=np.float32)
            keep = self.nms(boxes[:, :4], boxes[:, 4], 0.3)
            boxes = boxes[keep, :]
            lms = np.asarray(lms, dtype=np.float32)
            lms = lms[keep, :]
        return boxes, lms

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
    model_path = 'Models/centerface_1x3xHxW_original.onnx'
    #model_path = 'Models/centerface_1x3xHxW_PerChannel_quant_random_2.onnx' # Not working yet, actually not detecting any face. Obtained from ST platform
    threshold = 0.3

    centerface = CenterFace(model_path)

    # Use default webcam (index 0)
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        t = time.time()
                
        # # convert frame to grayscale
        # frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        # # replicate the grayscale image to 3 channels
        # frame = cv2.merge([frame, frame, frame])
        
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
    
    # Create directory to save frames
    output_dir = "captured_frames_onnx"
    os.makedirs(output_dir, exist_ok=True)
    
    #print(f"Saving {len(my_frames)} frames to {output_dir}")
    
    # # Save each frame
    # for i, frame in enumerate(my_frames):
    #     # Convert from NCHW to HWC format
    #     img = np.squeeze(frame)  # Remove batch dimension
    #     img = np.transpose(img, (1, 2, 0))  # CHW to HWC
        
    #     # Convert from float to uint8 range [0, 255]
    #     img = cv2.normalize(img, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)
        
    #     # Save the image
    #     filename = os.path.join(output_dir, f"frame_{i:04d}.png")
    #     cv2.imwrite(filename, img)
    
    # print(f"Successfully saved {len(my_frames)} frames to {output_dir}")


if __name__ == '__main__':
    
    main()
