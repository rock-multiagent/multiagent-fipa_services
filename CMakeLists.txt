# CMakeLists.txt has to be located in the project folder and cmake has to be
# executed from 'project/build' with 'cmake ../'.
cmake_minimum_required(VERSION 3.5)
project(fipa_services VERSION 0.1)
find_package(Rock)
rock_init()
rock_standard_layout()

