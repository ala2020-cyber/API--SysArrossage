// server.js
const express = require("express");
const mongoose = require("mongoose");
const bodyParser = require("body-parser");

const app = express();
app.use(bodyParser.json());

mongoose.connect(
  "mongodb+srv://user:user@cluster0.k8dtyrj.mongodb.net/SysArrosage?retryWrites=true&w=majority&appName=Cluster0",
  {
    useNewUrlParser: true,
    useUnifiedTopology: true,
  }
);

const sensorDataSchema = new mongoose.Schema({
  temperatureMeteo: Number,
  humidityMeteo: Number,
  humidityCapteur: Number,
  timestamp: { type: Date, default: Date.now },
});

const SensorData = mongoose.model("SensorData", sensorDataSchema);

app.post("/api/sensor-data", async (req, res) => {
  const { temperatureMeteo, humidityMeteo, humidityCapteur } = req.body;

  const newSensorData = new SensorData({
    temperatureMeteo,
    humidityMeteo,
    humidityCapteur,
  });
  await newSensorData.save();

  res.status(201).send("Data saved");
});

app.listen(3000, "0.0.0.0", () => {
  console.log("Server running on port 3000");
});
