#!/usr/bin/env python3
"""Fixed utility functions for UART image streaming with timeout protection."""

import numpy as np
import cv2
import time
import threading
import logging

logger = logging.getLogger(__name__)

def _search_header_with_timeout(ser, prefixes, timeout=2.0, max_attempts=50):
    """
    Read from serial until a line starting with any of `prefixes` is found.
    Returns the full line (including the prefix), or None on timeout/EOF.
    
    Args:
        ser: Serial port object
        prefixes: String or tuple of prefixes to search for
        timeout: Maximum time to wait in seconds
        max_attempts: Maximum number of read attempts
    """
    # normalize to a tuple of str
    if isinstance(prefixes, str):
        prefixes = (prefixes,)
    else:
        prefixes = tuple(prefixes)

    start_time = time.time()
    attempts = 0
    
    # Store original timeout and set a short timeout for non-blocking reads
    original_timeout = ser.timeout
    ser.timeout = 0.1  # 100ms timeout for each readline
    
    try:
        while attempts < max_attempts and (time.time() - start_time) < timeout:
            attempts += 1
            
            try:
                raw = ser.readline()
                if not raw:
                    # No data available, small sleep to prevent busy waiting
                    time.sleep(0.01)
                    continue

                # decode as ASCII, silently drop non-ASCII bytes
                line = raw.decode('ascii', errors='ignore').strip()
                
                # Skip empty lines
                if not line:
                    continue

                # C-level startswith check against all prefixes at once
                if line.startswith(prefixes):
                    return line
                    
            except Exception as e:
                logger.debug(f"Error reading line: {e}")
                time.sleep(0.01)
                continue
                
        # Timeout or max attempts reached
        logger.debug(f"Header search timeout after {attempts} attempts, {time.time() - start_time:.2f}s")
        return None
        
    finally:
        # Restore original timeout
        ser.timeout = original_timeout

def _search_header(ser, prefixes):
    """
    Legacy wrapper for compatibility - uses timeout protection
    """
    return _search_header_with_timeout(ser, prefixes, timeout=2.0)

def read_frame_with_timeout(ser, timeout=3.0):
    """Read a frame from the serial port with timeout protection.

    Returns ``(tag, image, width, height)`` or ``(None, None, None, None)`` if no frame is
    available or timeout occurs. The function searches the stream for ``JPG`` or ``ALN`` headers."""

    start_time = time.time()
    
    try:
        line = _search_header_with_timeout(ser, ("JPG", "ALN"), timeout=timeout)
        if line is None:
            return None, None, None, None

        # Parse header line
        try:
            parts = line.split()
            if len(parts) < 4:
                logger.warning(f"Invalid header format: {line}")
                return None, None, None, None
                
            tag, w, h, size = parts[:4]
            w, h, size = int(w), int(h), int(size)
            
            # Sanity check
            if size <= 0 or size > 10 * 1024 * 1024:  # Max 10MB
                logger.warning(f"Invalid frame size: {size}")
                return None, None, None, None
                
        except (ValueError, IndexError) as e:
            logger.warning(f"Error parsing header: {e}")
            return None, None, None, None

        # Read frame data with timeout
        bytes_read = 0
        frame_data = bytearray()
        read_timeout = min(timeout, 5.0)  # Max 5 seconds for frame data
        
        original_timeout = ser.timeout
        ser.timeout = 0.1  # Short timeout for incremental reads
        
        try:
            while bytes_read < size and (time.time() - start_time) < read_timeout:
                remaining = size - bytes_read
                chunk = ser.read(min(remaining, 1024))  # Read in chunks
                
                if chunk:
                    frame_data.extend(chunk)
                    bytes_read += len(chunk)
                else:
                    time.sleep(0.01)  # Small delay if no data
                    
        finally:
            ser.timeout = original_timeout
            
        if bytes_read < size:
            logger.warning(f"Incomplete frame data: {bytes_read}/{size} bytes")
            return None, None, None, None

        # Decode image
        try:
            img = np.frombuffer(frame_data, dtype=np.uint8)
            frame = cv2.imdecode(img, cv2.IMREAD_COLOR)
            
            if frame is None:
                logger.warning("Failed to decode frame data")
                return None, None, None, None
                
            return tag, frame, w, h
            
        except Exception as e:
            logger.warning(f"Error decoding frame: {e}")
            return None, None, None, None
            
    except Exception as e:
        logger.error(f"Error in read_frame_with_timeout: {e}")
        return None, None, None, None

def read_frame(ser):
    """Legacy wrapper for compatibility"""
    return read_frame_with_timeout(ser, timeout=3.0)

def read_detections_with_timeout(ser, timeout=2.0):
    """Read detection results for the current frame with timeout protection.

    Returns ``(frame_id, detections)``. The function searches for the ``DETS``
    header to avoid losing synchronization with the stream."""
    
    try:
        line = _search_header_with_timeout(ser, "DETS", timeout=timeout)
        if line is None:
            return None, []

        parts = line.split()
        if len(parts) < 3:
            return None, []

        frame_id = int(parts[1])
        count = int(parts[2])
        
        # Sanity check
        if count < 0 or count > 100:  # Max 100 detections
            logger.warning(f"Invalid detection count: {count}")
            return frame_id, []
        
        dets = []
        
        # Set timeout for detection reading
        original_timeout = ser.timeout
        ser.timeout = 0.5  # 500ms per detection line
        
        try:
            for i in range(count):
                try:
                    line = ser.readline().decode(errors="ignore").strip()
                    if not line:
                        continue
                        
                    tokens = line.split()
                    if len(tokens) < 6:
                        continue
                        
                    c, xc, yc, w, h, conf, *kp = tokens
                    keypoints = [float(v) for v in kp]
                    dets.append(
                        (
                            int(c),
                            float(xc),
                            float(yc),
                            float(w),
                            float(h),
                            float(conf),
                            keypoints,
                        )
                    )
                except (ValueError, IndexError) as e:
                    logger.debug(f"Error parsing detection {i}: {e}")
                    continue
                    
        finally:
            ser.timeout = original_timeout

        # Read END marker with timeout
        try:
            ser.timeout = 0.2
            ser.readline()  # END marker
        except:
            pass
        finally:
            ser.timeout = original_timeout
            
        return frame_id, dets
        
    except Exception as e:
        logger.error(f"Error in read_detections_with_timeout: {e}")
        return None, []

def read_detections(ser):
    """Legacy wrapper for compatibility"""
    return read_detections_with_timeout(ser, timeout=2.0)

def read_embedding_with_timeout(ser, timeout=2.0):
    """Read an embedding array sent by the MCU with timeout protection."""
    try:
        line = _search_header_with_timeout(ser, "EMB", timeout=timeout)
        if line is None:
            return []
            
        parts = line.split()
        if len(parts) < 2:
            return []
            
        count = int(parts[1])
        
        # Sanity check
        if count <= 0 or count > 1024:  # Max 1024 values
            logger.warning(f"Invalid embedding count: {count}")
            return []
        
        values = []
        
        # Set timeout for data reading
        original_timeout = ser.timeout
        ser.timeout = 1.0  # 1 second for embedding data
        
        try:
            if count > 0:
                data_line = ser.readline().decode(errors="ignore").strip()
                for tok in data_line.split():
                    if len(values) >= count:
                        break
                    try:
                        values.append(float(tok))
                    except ValueError:
                        pass
                        
        finally:
            ser.timeout = original_timeout
            
        # Read END marker with timeout
        try:
            ser.timeout = 0.2
            ser.readline()  # END marker
        except:
            pass
        finally:
            ser.timeout = original_timeout
            
        return values
        
    except Exception as e:
        logger.error(f"Error in read_embedding_with_timeout: {e}")
        return []

def read_embedding(ser):
    """Legacy wrapper for compatibility"""
    return read_embedding_with_timeout(ser, timeout=2.0)

def read_embeddings(ser, count):
    """Read *count* embeddings in a row with timeout protection."""
    embs = []
    start_time = time.time()
    
    for i in range(count):
        # Overall timeout of 10 seconds for all embeddings
        if time.time() - start_time > 10.0:
            logger.warning(f"Timeout reading embeddings after {i}/{count}")
            break
            
        emb = read_embedding_with_timeout(ser, timeout=2.0)
        if not emb:
            break
        embs.append(emb)
        
    return embs

def read_aligned_frames(ser, count):
    """Read *count* aligned frames from the MCU with timeout protection."""
    frames = []
    start_time = time.time()
    
    for i in range(count):
        # Overall timeout of 30 seconds for all frames
        if time.time() - start_time > 30.0:
            logger.warning(f"Timeout reading aligned frames after {i}/{count}")
            break
            
        tag, frame, _, _ = read_frame_with_timeout(ser, timeout=5.0)
        if tag != "ALN":
            break
        frames.append(frame)
        
    return frames

def draw_detections(img, dets, color=(0, 255, 0)):
    """Draw detection boxes on an image.

    *color* selects the rectangle and text color.
    """
    if img is None or len(img.shape) != 3:
        return img
        
    h, w, _ = img.shape
    for d in dets:
        if len(d) < 6:
            continue
        try:
            # support optional keypoints at index 6
            _, xc, yc, ww, hh, conf = d[:6]
            x0 = int((xc - ww / 2) * w)
            y0 = int((yc - hh / 2) * h)
            x1 = int((xc + ww / 2) * w)
            y1 = int((yc + hh / 2) * h)
            
            # Clamp coordinates
            x0 = max(0, min(w-1, x0))
            y0 = max(0, min(h-1, y0))
            x1 = max(0, min(w-1, x1))
            y1 = max(0, min(h-1, y1))
            
            cv2.rectangle(img, (x0, y0), (x1, y1), color, 2)
            cv2.putText(
                img,
                f"{conf:.2f}",
                (x0, y0 - 5),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                color,
                1,
            )
        except Exception as e:
            logger.debug(f"Error drawing detection: {e}")
            continue
            
    return img

def display_loop(q, stop_event):
    """Display frames from a queue with FPS counter and timeout protection."""
    scale = 2
    last_time = time.time()
    frame_count = 0
    fps = 0.0

    while not stop_event.is_set():
        try:
            # Use timeout to prevent blocking indefinitely
            frame = q.get(timeout=1.0)
            if frame is None:
                break

            frame_count += 1
            current_time = time.time()
            elapsed = current_time - last_time
            if elapsed >= 1.0:
                fps = frame_count / elapsed
                frame_count = 0
                last_time = current_time

            resized = cv2.resize(frame, (0, 0), fx=scale, fy=scale)
            cv2.putText(resized, f"FPS: {fps:.2f}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 0), 2)
            cv2.imshow('stream', resized)
            if cv2.waitKey(1) == 27:
                stop_event.set()
                
        except Exception as e:
            if not stop_event.is_set():
                logger.debug(f"Display loop error: {e}")
            time.sleep(0.1)

    cv2.destroyAllWindows()

def send_image_with_timeout(ser, img_path, size, display=False, rx=False, preview=False, timeout=10.0):
    """Send an image file to the board with timeout protection.

    Returns ``(frame, detections, aligned_frames, embeddings)`` when ``rx`` is
    ``True``. If *display* is True, the echoed frame with detection boxes is
    shown using OpenCV."""

    try:
        img = cv2.imread(img_path)
        if img is None:
            print(f"Failed to read {img_path}")
            return None, [], [], []

        img = cv2.resize(img, size)
        if preview:
            cv2.imshow("nn_in", img)
            cv2.waitKey(1)
            
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        ser.write(img.tobytes())

        if rx:
            old_timeout = ser.timeout
            start_time = time.time()
            
            try:
                ser.timeout = 1.0  # 1 second timeout for reads
                time.sleep(0.5)

                aligned = []
                embeddings = []
                echo = None
                max_frames = 10  # Prevent infinite loop
                
                frame_count = 0
                while frame_count < max_frames and (time.time() - start_time) < timeout:
                    frame_count += 1
                    
                    tag, frame, w, h = read_frame_with_timeout(ser, timeout=2.0)
                    if tag is None:
                        break
                        
                    if tag == "ALN":
                        aligned.append(frame)
                        emb = read_embedding_with_timeout(ser, timeout=2.0)
                        embeddings.append(emb)
                    else:
                        echo = frame
                        print('rxed frame')
                        break

                _, dets = read_detections_with_timeout(ser, timeout=2.0)
                print('rxed dets')

                if echo is not None:
                    drawn = draw_detections(echo.copy(), dets)
                    if display:
                        cv2.imshow("send_result", drawn)
                        cv2.waitKey(1)
                else:
                    print("No echo frame received")

                return echo, dets, aligned, embeddings
                
            finally:
                ser.timeout = old_timeout
                
    except Exception as e:
        logger.error(f"Error in send_image_with_timeout: {e}")
        return None, [], [], []

def send_image(ser, img_path, size, display=False, rx=False, preview=False, timeout=10.0):
    """Legacy wrapper for compatibility"""
    return send_image_with_timeout(ser, img_path, size, display, rx, preview, timeout)