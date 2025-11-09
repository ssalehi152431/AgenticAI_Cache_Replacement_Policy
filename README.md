# csc591s25_final_project

## Setup

1. Create a `.env` file using the provided template `.env.example`.
2. Install dependencies:
   ```bash
   pip install -r requirements.txt
3. Run `main.py`




# 🧠 AgenticAI Cache Replacement Policy

![Python](https://img.shields.io/badge/Python-3.x-blue.svg)
![C++](https://img.shields.io/badge/C++-76%25-orange.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Status](https://img.shields.io/badge/Status-Active-brightgreen.svg)
---

## 🔍 Overview
**AgenticAI Cache Replacement Policy** is an AI-driven framework for optimizing cache performance through adaptive decision-making.  
It leverages agentic AI (e.g., reinforcement learning or genetic algorithms) to learn optimal eviction strategies, outperforming traditional static policies such as LRU and LFU.

The system integrates a **C++ cache simulator** for speed and realism with **Python-based AI modules** for intelligent control and analysis.

---

## 💡 Motivation
Conventional cache replacement strategies rely on fixed heuristics, making them ineffective under dynamic and data-intensive workloads.  
This project introduces a **self-learning AI agent** that dynamically adapts cache eviction behavior to changing access patterns—enhancing hit rates, minimizing latency, and improving computational efficiency.

---

## 📂 Project Structure
├── Code/ # Simulator and agent code (C++ and Python)
├── Dataset/ # Access traces or synthetic workloads
├── logs/ # Training & evaluation logs
├── Report_GenAI_CacheReplacementPolicy.pdf
├── Persona Prompt.txt
├── score_record.txt
├── requirements.txt
└── README.md




**Languages Used:**  
- 🟠 **C++ (~76.8%)** — Cache simulator core  
- 🔵 **Python (~23.2%)** — Agent logic, training, evaluation  

---

## ⚙️ Setup & Installation
# Clone the repository
git clone https://github.com/ssalehi152431/AgenticAI_Cache_Replacement_Policy.git
cd AgenticAI_Cache_Replacement_Policy

# (Optional) Create a virtual environment
python3 -m venv venv
source venv/bin/activate      # On macOS/Linux
venv\Scripts\activate         # On Windows

# Install Python dependencies
pip install -r requirements.txt

# Build or compile the C++ simulator (if needed)
make



▶️ Usage

Run baseline and AI-enhanced cache simulations with configurable parameters.
# Run baseline cache simulator
./simulator --trace Dataset/workload1.trace --cache_size 128K --assoc 4

# Train the AI agent
python train_agent.py --epochs 1000 --trace Dataset/train_trace1.trace

# Evaluate trained agent
python eval_agent.py --trace Dataset/test_trace2.trace --output logs/test2.log


