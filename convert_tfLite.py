import tensorflow as tf

model = tf.keras.models.load_model(r"C:\Users\biwla\AppData\Local\Programs\Microsoft VS Code\gesture_model.h5")
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Save .tflite
with open("gesture_model.tflite", "wb") as f:
    f.write(tflite_model)

print("✅ gesture_model.tflite saved!")
