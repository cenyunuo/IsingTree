#include <pybind11/pybind11.h>
#include "IsingTree.hpp"  // Include the correct header file for Solver

namespace py = pybind11;

void init_solver(py::module_ &m) {
    py::class_<Solver>(m, "Solver")
        .def(py::init<>())
        .def("solve_limited", &Solver::solve_limited)
        .def("assign", &Solver::assign)
        .def("propagate", &Solver::propagate)
        .def("backtrack", &Solver::backtrack)
        .def("analyze", &Solver::analyze)
        .def("parse", &Solver::parse)
        .def("add_clause", &Solver::add_clause)
        .def("setThreshold", &Solver::setThreshold)
        .def("getModel", &Solver::getModel)
        .def("getActivity", &Solver::getActivity)
        .def("getHardClause", &Solver::getHardClause)
        .def("getSoftClause", &Solver::getSoftClause)
        .def("fromIsing", &Solver::fromIsing);
}

PYBIND11_MODULE(IsingTree, m) {
    m.doc() = "pybind11 example plugin";  // Optional documentation
    init_solver(m);
}
