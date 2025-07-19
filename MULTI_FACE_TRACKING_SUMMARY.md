# Multi-Face Tracking Implementation Summary

## Problem Solved
The original system suffered from similarity "smoothing" jumps between different faces when multiple people were in frame. The system would constantly switch between faces, causing inconsistent recognition results.

## Solution Implemented

### 1. Face Tracking Structures
```c
typedef struct {
    pd_pp_box_t box;                 // Tracked face bounding box
    float similarity_history[5];     // Similarity history for this face
    uint32_t history_index;          // Current index in circular buffer
    uint32_t history_count;          // Number of valid history entries
    float smoothed_similarity;       // Smoothed similarity score
    bool active;                     // Face tracker is active
    uint32_t last_seen_frame;        // Last frame this face was detected
    uint32_t face_id;                // Unique face identifier
} face_tracker_t;

typedef struct {
    face_tracker_t trackers[MAX_TRACKED_FACES];
    uint32_t next_face_id;
    uint32_t current_frame;
    int primary_face_idx;            // Index of primary face for recognition
} face_tracking_context_t;
```

### 2. Key Functions Implemented

#### `calculate_box_iou()`
- Computes Intersection over Union between two bounding boxes
- Used to match detections with existing trackers
- Threshold: 0.3 IoU for face matching

#### `update_face_tracking()`
- Updates all face trackers with new detections
- Matches detections to existing trackers using IoU
- Creates new trackers for unmatched faces
- Expires trackers not seen for 5+ frames

#### `select_primary_face()`
- Chooses the best face for recognition processing
- Considers detection confidence + tracking stability
- Applies hysteresis to prevent jumping between faces
- Prefers current primary face to maintain consistency

#### `update_tracker_similarity()` & `compute_tracker_stability()`
- Each face maintains its own similarity history buffer
- Prevents cross-contamination between different people
- Individual smoothing and variance calculation per face

### 3. Processing Flow

1. **Frame Processing**: `process_frame_detections()`
   - Updates face tracking with new detections
   - Selects primary face for recognition
   - Runs face recognition only on primary face
   - Updates similarity history for primary face only

2. **Multi-Face Handling**:
   - Up to 3 faces can be tracked simultaneously
   - Each face gets unique Face ID for debugging
   - Primary face gets full recognition processing
   - Secondary faces are visually indicated with lower confidence

3. **Consistency Mechanisms**:
   - IoU-based face matching prevents tracker confusion
   - Hysteresis in primary face selection reduces jumping
   - Per-face similarity smoothing prevents cross-contamination
   - Timeout mechanism handles face appearance/disappearance

### 4. Key Benefits

- **Stable Recognition**: Primary face selection with hysteresis
- **Individual Tracking**: Each face maintains separate similarity history
- **Robust Matching**: IoU-based association handles face movement
- **Graceful Handling**: Automatic face timeout and new face detection
- **Visual Feedback**: Face IDs and tracking status for debugging

### 5. Configuration Parameters

```c
#define MAX_TRACKED_FACES 3           // Maximum simultaneous faces
#define FACE_MATCH_THRESHOLD 0.3f     // IoU threshold for matching
#define FACE_LOST_TIMEOUT_FRAMES 5    // Frames before face timeout
```

### 6. Integration Points

- Modified `app_context_t` to include `face_tracking_context_t`
- Updated `process_frame_detections()` to use tracking system
- Enhanced logging to show Face IDs and tracking status
- Maintained backward compatibility with existing LCD/PC stream output

## Result
The system now provides stable, consistent face recognition in multi-face scenarios by:
1. Tracking individual faces across frames
2. Selecting a consistent primary face for recognition
3. Preventing similarity score contamination between different people
4. Providing smooth transitions when faces appear/disappear

This eliminates the problematic "jumping" behavior and provides reliable recognition results even with multiple people in the frame.