/*=========================================================================

  Program:   Visualization Toolkit
  Module:    main.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkm/cont/Initialize.h"
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkmClip.h>
#include <vtksys/SystemInformation.hxx>

#include <chrono>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
  if (argc < 5)
  {
    std::cerr << "Usage: " << argv[0] << " filename percentage numberOfIterations numberOfThreads"
              << std::endl;
    return EXIT_FAILURE;
  }
  const std::string filename = argv[1];
  const double P = std::stod(argv[2]);
  const int numberOfIterations = std::stoi(argv[3]);
  const int numberOfThreads = std::stoi(argv[4]);

  std::vector<std::string> strings = { argv[0], "--vtkm-device", "TBB", "--vtkm-num-threads",
    std::to_string(numberOfThreads) };
  std::vector<char*> argvVector;
  for (const auto& str : strings)
  {
    argvVector.push_back(const_cast<char*>(str.c_str()));
  }
  int vtkm_argc = static_cast<int>(argvVector.size());
  char** vtkm_argv = argvVector.data();
  vtkm::cont::Initialize(vtkm_argc, vtkm_argv,
    vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::ErrorOnBadOption);

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  auto output = reader->GetOutput();
  double bounds[6];
  output->GetBounds(bounds);
  // double origin[3] = { (bounds[1] + bounds[0]) / 2.0, (bounds[3] + bounds[2]) / 2.0,
  //   (bounds[5] + bounds[4]) / 2.0 };
  double origin[3] = { bounds[0] + (1 - P) * (bounds[1] - bounds[0]), (bounds[3] + bounds[2]) / 2.0,
    (bounds[5] + bounds[4]) / 2.0 };
  std::cout << "Origin: " << origin[0] << ", " << origin[1] << ", " << origin[2] << std::endl;
  double normal[3] = { 1.0, 0.0, 0.0 };
  std::cout << "Normal: " << normal[0] << ", " << normal[1] << ", " << normal[2] << std::endl;

  std::cout << "Number of input cells: " << output->GetNumberOfCells() << std::endl;
  std::cout << "Number of input points: " << output->GetNumberOfPoints() << std::endl;

  vtksys::SystemInformation sysinfo;
  const auto memoryUsedBeforeFilter = sysinfo.GetProcMemoryUsed();
  std::cout << "Memory used by dataset in KB: " << memoryUsedBeforeFilter << std::endl;
  size_t totalTime = 0;
  for (int i = 0; i < numberOfIterations; ++i)
  {
    vtkNew<vtkPlane> plane;
    plane->SetOrigin(origin);
    plane->SetNormal(normal);
    auto start = std::chrono::high_resolution_clock::now();
    auto filter = vtkSmartPointer<vtkmClip>::New();
    filter->SetInputConnection(reader->GetOutputPort());
    filter->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
    filter->SetClipFunction(plane);
    filter->Update();
    auto end = std::chrono::high_resolution_clock::now();
    totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Number of output cells: " << filter->GetOutput()->GetNumberOfCells() << std::endl;
    std::cout << "Number of output points: " << filter->GetOutput()->GetNumberOfPoints()
              << std::endl;
    filter = nullptr;
  }
  std::cout << "Time: " << totalTime / numberOfIterations << std::endl;

  return EXIT_SUCCESS;
}
