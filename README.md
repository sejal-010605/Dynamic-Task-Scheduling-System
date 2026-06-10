# Prediction of Concrete Compressive Strength using Artificial Neural Networks (ANN)

## Overview

This project develops an Artificial Neural Network (ANN) model to predict the compressive strength of concrete based on its mix composition and curing age. The goal is to provide a fast, cost-effective, and accurate alternative to traditional laboratory testing methods used in civil engineering.

The model is trained on the Concrete Compressive Strength Dataset from the UCI Machine Learning Repository and evaluates multiple neural network configurations through hyperparameter optimization.

---

## Project Objectives

- Predict concrete compressive strength using Artificial Neural Networks.
- Analyze relationships between concrete constituents and strength.
- Perform Exploratory Data Analysis (EDA).
- Compare different activation functions and optimizers.
- Evaluate model performance using regression metrics.
- Improve prediction accuracy through hyperparameter tuning.

---

## Dataset

**Source:** UCI Machine Learning Repository

### Dataset Statistics

| Parameter | Value |
|------------|--------|
| Total Samples | 1030 |
| Input Features | 8 |
| Output Variables | 1 |

### Input Features

- Cement (kg/m³)
- Blast Furnace Slag (kg/m³)
- Fly Ash (kg/m³)
- Water (kg/m³)
- Superplasticizer (kg/m³)
- Coarse Aggregate (kg/m³)
- Fine Aggregate (kg/m³)
- Age (Days)

### Output Variable

- Concrete Compressive Strength (MPa)

---

## Data Preprocessing

### Data Cleaning

- Loaded dataset using Pandas.
- Simplified feature names.
- Verified absence of missing values.

### Exploratory Data Analysis

- Histograms for feature distributions.
- Box plots for outlier detection.
- Correlation heatmap analysis.

### Feature Scaling

Standardization was applied to:

- Normalize feature ranges.
- Improve ANN convergence.
- Enhance model stability.

### Data Splitting

| Dataset | Percentage |
|----------|------------|
| Training Set | 70% |
| Validation Set | 15% |
| Test Set | 15% |

---

## Model Architecture

### Baseline ANN Structure

```text
Input Layer (8 Features)
        ↓
Hidden Layer 1 (16 Neurons, ReLU)
        ↓
Hidden Layer 2 (16 Neurons, ReLU)
        ↓
Output Layer (1 Neuron, Linear Activation)
```

### Training Configuration

| Parameter | Value |
|------------|--------|
| Loss Function | Mean Squared Error (MSE) |
| Optimizer | Adam |
| Learning Rate | 0.001 |
| Batch Size | 32 |
| Epochs | 100 |
| Early Stopping | Enabled |

---

## Evaluation Metrics

The following regression metrics were used:

- Root Mean Squared Error (RMSE)
- Mean Absolute Error (MAE)
- R² Score (Coefficient of Determination)

---

## Hyperparameter Optimization

The following configurations were investigated:

### Number of Neurons

- 16
- 32
- 64

### Activation Functions

- ReLU
- Tanh
- Sigmoid

### Optimizers

- Adam
- SGD
- RMSprop
- Adagrad

### Learning Rate Scheduling

- Constant Learning Rate
- Step Decay
- Exponential Decay
- Cosine Decay

---

## Results

### Baseline Model Performance

| Metric | Value |
|---------|---------|
| RMSE | 9.79 MPa |
| MAE | 7.88 MPa |
| R² Score | 0.62 |

### Hyperparameter Optimization Results

| Model | Optimizer | Activation | RMSE | MAE | R² |
|---------|------------|------------|---------|---------|---------|
| Model 1 | Adam | ReLU | 9.90 | 8.03 | 0.609 |
| Model 2 | Adam | ReLU | 7.08 | 5.57 | 0.800 |
| Model 3 | SGD | ReLU | 6.16 | 4.93 | 0.848 |
| Model 4 | RMSprop | Tanh | 6.08 | 5.05 | 0.852 |
| Model 5 | Adagrad | Sigmoid | 31.62 | 27.48 | -2.99 |

### Key Findings

- ANN effectively captured nonlinear relationships between mix components and strength.
- Hyperparameter tuning significantly improved model performance.
- SGD and RMSprop delivered the best predictive accuracy.
- Adagrad performed poorly on this dataset.
- Prediction accuracy decreased slightly for extreme strength values.

---

## Project Workflow

```text
Dataset Collection
        ↓
Data Preprocessing
        ↓
Exploratory Data Analysis
        ↓
Feature Scaling
        ↓
Train-Validation-Test Split
        ↓
ANN Model Development
        ↓
Hyperparameter Optimization
        ↓
Model Evaluation
        ↓
Concrete Strength Prediction
```

---

## Technologies Used

- Python
- NumPy
- Pandas
- Matplotlib
- Seaborn
- Scikit-Learn
- TensorFlow
- Keras

---

## Future Scope

### Dataset Expansion

- Incorporate larger and more diverse datasets.
- Include environmental parameters such as temperature and humidity.

### Advanced Deep Learning Models

- Deep Neural Networks (DNN)
- Convolutional Neural Networks (CNN)
- Ensemble Learning Techniques

### IoT Integration

- Real-time strength prediction using embedded sensors.
- Continuous monitoring of concrete performance.

### Multi-Property Prediction

Extend the model to predict:

- Tensile Strength
- Modulus of Elasticity
- Durability Characteristics

### Deployment

- Web-based applications
- Mobile applications for field engineers
- Decision-support tools for construction projects

---

## Repository Structure

```text
├── data/
│   └── Concrete_Data.csv
├── notebooks/
│   └── ANN_Concrete_Strength.ipynb
├── src/
│   ├── preprocessing.py
│   ├── train.py
│   ├── evaluate.py
│   └── model.py
├── results/
│   ├── plots/
│   └── metrics/
├── requirements.txt
├── README.md
└── LICENSE
```

---

## Authors
**Sejal Yenage**  
**Deepika Yampati**



## References

1. UCI Machine Learning Repository – Concrete Compressive Strength Dataset
2. Applied Sciences, 2021, MDPI
3. Neural Computing and Applications
4. Advances in Civil Engineering
5. Related ANN-based concrete strength prediction literature

---


This project was developed for academic and educational purposes as part of the Smart Infrastructure Engineering course.
