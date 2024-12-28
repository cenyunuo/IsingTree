# IsingTree
A tree search algorithm for improving continuous Ising solvers on MaxSAT solving.

## Requirement
```
sudo apt install pybind11-dev
pip install -U jax
pip install jaxopt
```

## Installation
```
mkdir build
cd build
cmake ..
make
mv IsingTree.so ../
```

## Related Work
- The code is mainly modified from [EasySAT](https://github.com/shaowei-cai-group/EasySAT).
- The file parser is based on [NuWLS/NuWLS-source-code/build.h](https://github.com/filyouzicha/NuWLS/blob/main/NuWLS-source-code/build.h).
- Continuous Local Search: [FastFourierSAT (AAAI'25)](https://github.com/seeder-research/FastFourierSAT).
  ```
  @article{cen2023massively,
    title={Massively Parallel Continuous Local Search for Hybrid SAT Solving on GPUs},
    author={Cen, Yunuo and Zhang, Zhiwei and Fong, Xuanyao},
    journal={arXiv preprint arXiv:2308.15020},
    year={2023}
  }
  ```
- Tree Search: [CITS (Scientific Reports, 2022)](https://www.nature.com/articles/s41598-022-19102-x)
  ```
  @article{cen2022tree,
    title={A tree search algorithm towards solving Ising formulated combinatorial optimization problems},
    author={Cen, Yunuo and Das, Debasis and Fong, Xuanyao},
    journal={Scientific Reports},
    volume={12},
    number={1},
    pages={14755},
    year={2022},
    publisher={Nature Publishing Group UK London}
  }
  ```
