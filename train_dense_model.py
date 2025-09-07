import pandas as pd
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import confusion_matrix, classification_report
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Flatten
from tensorflow.keras.utils import to_categorical
from tensorflow.keras.preprocessing.sequence import pad_sequences
import matplotlib.pyplot as plt
import seaborn as sns
import pickle
import os

# === Load and merge ===
data = pd.read_csv(r"C:\Users\biwla\Documents\4th_Year\Data aqucision\Seeed Project\GestureData\gesture_data.csv")
labels = pd.read_csv(r"C:\Users\biwla\Documents\4th_Year\Data aqucision\Seeed Project\GestureData\gesture_labels.csv")
data = data.merge(labels, on="session_id")

# === Group by session ===
sequences = []
sequence_labels = []

for session_id, group in data.groupby("session_id"):
    group = group.drop_duplicates(subset=["ax", "ay", "az", "gx", "gy", "gz"])
    seq = group[["ax", "ay", "az", "gx", "gy", "gz"]].values
    sequences.append(seq)
    sequence_labels.append(group["label"].iloc[0])

# === Pad to fixed length ===
MAX_LEN = 50
sequences_padded = pad_sequences(sequences, maxlen=MAX_LEN, dtype='float32', padding='post')

# === Encode labels ===
encoder = LabelEncoder()
y_encoded = encoder.fit_transform(sequence_labels)
y_categorical = to_categorical(y_encoded)

# === Train/test split ===
X_train, X_test, y_train, y_test = train_test_split(sequences_padded, y_categorical, test_size=0.2, random_state=42)

# === Fully-connected model ===
model = Sequential([
    Flatten(input_shape=(MAX_LEN, 6)),
    Dense(64, activation='relu'),
    Dense(32, activation='relu'),
    Dense(len(encoder.classes_), activation='softmax')
])

model.compile(optimizer='adam', loss='categorical_crossentropy', metrics=['accuracy'])

# === Train ===
history = model.fit(X_train, y_train, epochs=50, batch_size=16, validation_split=0.2)

# === Evaluate ===
loss, acc = model.evaluate(X_test, y_test)
print(f"\n✅ Test Accuracy: {acc*100:.2f}%")

# === Confusion Matrix ===
y_pred_probs = model.predict(X_test)
y_pred = np.argmax(y_pred_probs, axis=1)
y_true = np.argmax(y_test, axis=1)

print("\n=== Classification Report ===")
print(classification_report(y_true, y_pred, target_names=encoder.classes_))

plt.figure(figsize=(6,5))
sns.heatmap(confusion_matrix(y_true, y_pred), annot=True, fmt='d', xticklabels=encoder.classes_, yticklabels=encoder.classes_)
plt.xlabel("Predicted")
plt.ylabel("Actual")
plt.title("Confusion Matrix")
plt.show()

# === Save model and encoder ===
model.save("gesture_model.h5")
with open("label_encoder.pkl", "wb") as f:
    pickle.dump(encoder, f)

print("Model saved at:", os.path.abspath("gesture_model.h5"))
