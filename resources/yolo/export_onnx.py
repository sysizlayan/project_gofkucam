from ultralytics import YOLO

# Load the YOLOv11n model
model = YOLO("yolo12x.pt")

# Export the model to ONNX format
model.export(format="onnx", opset=21)