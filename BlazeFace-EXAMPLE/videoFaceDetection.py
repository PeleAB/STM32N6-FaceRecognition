import pafy
import cv2
from BlazeFaceDetection.blazeFaceDetector import blazeFaceDetector

videoUrl = 'https://youtu.be/fLexgOxsZu0'
videoPafy = pafy.new(videoUrl)

scoreThreshold = 0.7
iouThreshold = 0.3
modelType = "front"

# Initialize face detector
faceDetector = blazeFaceDetector(modelType, scoreThreshold, iouThreshold)

# Initialize video
# cap = cv2.VideoCapture("img/test.mp4")
print(videoPafy.streams)
cap = cv2.VideoCapture(videoPafy.streams[-1].url)
cv2.namedWindow("detections", cv2.WINDOW_NORMAL) 
while cap.isOpened():

	# Read frame from the video
	ret, img = cap.read()

	if ret:	

		# Detect faces
		detectionResults = faceDetector.detectFaces(img)

		# Draw detections
		img_plot = faceDetector.drawDetections(img, detectionResults)
		cv2.imshow("detections", img_plot)

	# Press key q to stop
	if cv2.waitKey(1) == ord('q'):
		break

cap.release()
cv2.destroyAllWindows()
