## PerMA-Bench Evaluation Branch

This branch contains all results from our benchmarks and the scripts to create the plots from our paper.

#### Setup

The plots are generated with Python3.
Create a virtual environment and install the required packages.

```shell
$ python3 -m venv venv
$ source venv/bin/activate
$ pip install -r requirements.txt
```

#### Creating Plots

The results are stored in the `results` directory.
Each file is named after the configuration in the paper.

The `scripts` directory contains all the visualization scripts to create the plots shown in the paper.