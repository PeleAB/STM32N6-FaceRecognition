import time
import cv2
import numpy as np

import tensorflow as tf

Interpreter = tf.lite.Interpreter


class CenterFace(object):
    def __init__(self, model_path: str, modelinputshape=[128, 128]):
        # 1) Initialize TFLite interpreter
        self.interpreter = Interpreter(model_path=model_path)
        self.interpreter.allocate_tensors()

        # 2) Retrieve input/output details
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()

        self.modelinputshape = modelinputshape

        # 3) Map outputs to the correct indices
        if 'integer' in model_path:
            # print("Integer model")
            self.idx_heatmap = 2
            self.idx_scale = 0
            self.idx_offset = 3
            self.idx_lms = 1
        else:
            self.idx_heatmap = 0
            self.idx_scale = 1  # the order of scale and offset are correct here
            self.idx_offset = 2
            self.idx_lms = 3

    def transform(self, h, w):
        """
        Model expects 128*128. We record scale factors to map back to original size.
        """
        img_h_new, img_w_new = self.modelinputshape
        scale_h = img_h_new / h
        scale_w = img_w_new / w
        return img_h_new, img_w_new, scale_h, scale_w

    def inference(self, img, threshold):
        """
        Preprocess, run TFLite inference, and postprocess the outputs.
        """
        h, w = img.shape[:2]
        # We'll track these so we can map boxes/landmarks back to original size
        img_h_new, img_w_new, scale_h, scale_w = self.transform(h, w)

        input_image = cv2.dnn.blobFromImage(
            img, scalefactor=1.0,
            size=(img_w_new, img_h_new),
            mean=(0, 0, 0),
            swapRB=True,
            crop=False
        )
        print("[INFO] inference started...")
        print(input_image)
        # input_image = np.transpose(input_image, (0, 2, 3, 1))
        # input_image= input_image.astype(np.int8)

        # Set input tensor
        self.interpreter.set_tensor(self.input_details[0]['index'], input_image)

        # Inference
        self.interpreter.invoke()

        # Retrieve outputs
        heatmap = self.interpreter.get_tensor(self.output_details[self.idx_heatmap]['index'])
        scale = self.interpreter.get_tensor(self.output_details[self.idx_scale]['index'])
        offset = self.interpreter.get_tensor(self.output_details[self.idx_offset]['index'])
        lms = self.interpreter.get_tensor(self.output_details[self.idx_lms]['index'])

        # Decode & rescale back to original image size
        return self.postprocess(
            heatmap, lms, offset, scale,
            scale_h, scale_w,
            h, w,  # original size
            threshold
        )

    def postprocess(self, heatmap, lms, offset, scale,
                    scale_h, scale_w, orig_h, orig_w, threshold):
        """
        Decode in x*x space, then scale boxes/landmarks up to the original frame.
        """
        # Pass the (32,32) size to decode, because that’s the domain the network used.
        dets, lms_pts = self.decode(
            heatmap, scale, offset, lms,
            (self.modelinputshape[0] // 4, self.modelinputshape[1] // 4),  # decode in 32×32 domain
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

        heatmap = heatmap[0]
        scale = scale[0]
        offset = offset[0]
        landmark = landmark[0]

        heatmap = heatmap[..., 0]
        scale0 = scale[..., 0]
        scale1 = scale[..., 1]
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

                x1 = max(0, (x_idx + o1 + 0.5) * 4 - s1 / 2)
                y1 = max(0, (y_idx + o0 + 0.5) * 4 - s0 / 2)
                x2 = max(x1 + s1, size[1])
                y2 = max(y1 + s0, size[0])

                boxes.append([x1, y1, x2, y2, score])

                # this is the code from demo_onnx.py:

                # x1, y1 = max(0, (c1[i] + o1 + 0.5) * 4 - s1 / 2), max(0, (c0[i] + o0 + 0.5) * 4 - s0 / 2)
                # x1, y1 = min(x1, size[1]), min(y1, size[0])
                # boxes.append([x1, y1, min(x1 + s1, size[1]), min(y1 + s0, size[0]), score])

                lms_temp = []
                for j in range(5):
                    lm_y = landmark[y_idx, x_idx, j * 2 + 0]
                    lm_x = landmark[y_idx, x_idx, j * 2 + 1]
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
        """
        Standard NMS
        """
        x1 = boxes[:, 0]
        y1 = boxes[:, 1]
        x2 = boxes[:, 2]
        y2 = boxes[:, 3]
        areas = (x2 - x1 + 1) * (y2 - y1 + 1)
        order = np.argsort(scores)[::-1]
        num_detections = boxes.shape[0]
        suppressed = np.zeros((num_detections,), dtype=bool)

        keep = []
        for _i in range(num_detections):
            i = order[_i]
            if suppressed[i]:
                continue
            keep.append(i)

            ix1, iy1, ix2, iy2 = x1[i], y1[i], x2[i], y2[i]
            iarea = areas[i]

            for _j in range(_i + 1, num_detections):
                j = order[_j]
                if suppressed[j]:
                    continue
                xx1 = max(ix1, x1[j])
                yy1 = max(iy1, y1[j])
                xx2 = min(ix2, x2[j])
                w = max(0, xx2 - xx1 + 1)
                h = max(0, xx2 - xx1 + 1)

                inter = w * h
                ovr = inter / (iarea + areas[j] - inter)
                if ovr >= nms_thresh:
                    suppressed[j] = True

        return keep


def main():
    # model_path = 'Models/model_integer_quant.tflite' # output totally garbage
    # model_path = 'Models/model_float16_quant.tflite' #works but very poorly
    # model_path = 'Models/model_weight_quant.tflite' # works but very poorly
    # model_path = 'Models/model_float32.tflite' #works very poorly
    # model_path = "Models/centerface_1x3x128x128_integer_quant.tflite" #not working
    # model_path = "Models/centerface_1x3x128x128_full_integer_quant.tflite"
    # model_path = "Models/centerface_1x3xHxW_dynamic_range_quant.tflite"
    # model_path = "Models/centerface_1x3xHxW_full_integer_quant.tflite"

    model_path = "./Models/centerface_1x3xHxW_integer_quant.tflite"  # this the qunatized first version model using the webcame calib data that works well

    threshold = 0.50

    centerface = CenterFace(model_path)

    cap = cv2.VideoCapture('sample.mp4')
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        t = time.time()
        time.sleep(1 / 30.0)
        # Get frame dimensions
        height, width = frame.shape[:2]

        # Find the minimum dimension
        min_dim = min(height, width)

        # Calculate crop coordinates (centered)
        x_start = (width - min_dim) // 2
        y_start = (height - min_dim) // 2
        x_end = x_start + min_dim
        y_end = y_start + min_dim

        # Crop the square patch
        square_frame = frame[y_start:y_end, x_start:x_end]

        # Use the square frame for inference
        frame = square_frame

        # resize the frame to 128x128
        # frame = cv2.resize(frame, (128, 128))

        # # convert frame to grayscale
        # frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        # # replicate the grayscale image to 3 channels
        # frame = cv2.merge([frame, frame, frame])

        dets, lms = centerface.inference(frame, threshold)
        dt = time.time() - t + 0.001
        fps = 1.0 / dt

        # Draw detections and landmarks
        for det in dets:
            boxes, score = det[:4], det[4]
            cv2.rectangle(
                frame,
                (int(boxes[0]), int(boxes[1])),
                (int(boxes[2]), int(boxes[3])),
                (2, 255, 0),
                2
            )
            score_text = f"{score:.5f}"
            cv2.putText(
                frame, score_text,
                (int(boxes[0]), int(boxes[1]) - 5),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5,
                (0, 255, 0), 1
            )

        for lm in lms:
            for i in range(5):
                px = int(lm[2 * i])
                py = int(lm[2 * i + 1])
                cv2.circle(frame, (px, py), 2, (0, 0, 255), -1)

        # Add FPS text
        cv2.putText(frame, f"FPS: {fps:.2f}", (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 1,
                    (0, 255, 0), 2)

        cv2.imshow('centerface_tflite', frame)
        key = cv2.waitKey(1)
        if key == 27:  # ESC
            break

    cap.release()
    cv2.destroyAllWindows()


def process_video(model_path, video_path, detection_result_path=None, threshold=0.5, show_frames=False):
    """
    Process a video file with CenterFace detection and save the annotated result.

    Args:
        model_path (str): Path to the TFLite model
        video_path (str): Path to the input video
        detection_result_path (str, optional): Path to save the output video. If None,
                                              saves to current directory.
        threshold (float): Detection confidence threshold
        show_frames (bool): Whether to display frames during processing
    """
    import os

    # Initialize the detector with the model
    centerface = CenterFace(model_path)

    # Open the input video
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"Error: Could not open video file {video_path}")
        return

    # Get video properties
    fps = cap.get(cv2.CAP_PROP_FPS)
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    # Set up output path
    if detection_result_path is None:
        # Extract the filename from the input path
        input_filename = os.path.basename(video_path)
        name, ext = os.path.splitext(input_filename)
        detection_result_path = f"{name}_annotated{ext}"

    # Create video writer
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(detection_result_path, fourcc, fps, (width, height))

    frame_count = 0
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame_count += 1
        if frame_count % 100 == 0:
            print(f"Processed {frame_count} frames")

        # Store original frame for output
        original_frame = frame.copy()

        # Get frame dimensions
        frame_height, frame_width = frame.shape[:2]

        # Find the minimum dimension for square crop (as in the original code)
        min_dim = min(frame_height, frame_width)

        # Calculate crop coordinates (centered)
        x_start = (frame_width - min_dim) // 2
        y_start = (frame_height - min_dim) // 2
        x_end = x_start + min_dim
        y_end = y_start + min_dim

        # Crop the square patch for inference
        square_frame = frame[y_start:y_end, x_start:x_end]

        t = time.time()
        # Run detection on the square crop
        dets, lms = centerface.inference(square_frame, threshold)
        dt = time.time() - t
        fps_val = 1.0 / dt

        # Adjust coordinates back to the original frame
        if len(dets) > 0:
            # Adjust bounding boxes
            for i in range(len(dets)):
                dets[i][0] += x_start  # x1
                dets[i][1] += y_start  # y1
                dets[i][2] += x_start  # x2
                dets[i][3] += y_start  # y2

            # Adjust landmarks
            for i in range(len(lms)):
                for j in range(5):
                    lms[i][2 * j] += x_start  # x coordinate
                    lms[i][2 * j + 1] += y_start  # y coordinate

        # Draw detections and landmarks on the original frame
        for det in dets:
            boxes, score = det[:4], det[4]
            cv2.rectangle(
                original_frame,
                (int(boxes[0]), int(boxes[1])),
                (int(boxes[2]), int(boxes[3])),
                (2, 255, 0),
                2
            )
            score_text = f"{score:.5f}"
            cv2.putText(
                original_frame, score_text,
                (int(boxes[0]), int(boxes[1]) - 5),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5,
                (0, 255, 0), 1
            )

        for lm in lms:
            for i in range(5):
                px = int(lm[2 * i])
                py = int(lm[2 * i + 1])
                cv2.circle(original_frame, (px, py), 2, (0, 0, 255), -1)

        # Add FPS text
        cv2.putText(original_frame, f"FPS: {fps_val:.2f}", (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 1,
                    (0, 255, 0), 2)

        # Show the frame if requested
        if show_frames:
            cv2.imshow('Processing Video', original_frame)
            key = cv2.waitKey(1)
            if key == 27:  # ESC
                break

        # Write the frame to output video
        out.write(original_frame)

    # Release resources
    cap.release()
    out.release()
    if show_frames:
        cv2.destroyAllWindows()

    print(f"Video processing complete. Output saved to: {detection_result_path}")


if __name__ == '__main__':
    main()