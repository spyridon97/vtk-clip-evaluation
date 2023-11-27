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

#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkSMPTools.h>
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

  if (numberOfThreads <= 1)
  {
    vtkSMPTools::SetBackend("SEQUENTIAL");
    vtkSMPTools::Initialize(1);
  }
  else
  {
    vtkSMPTools::SetBackend("TBB");
    vtkSMPTools::Initialize(numberOfThreads);
  }

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  auto output = reader->GetOutput();
  double bounds[6];
  output->GetBounds(bounds);
  // double origin[3] = { (bounds[1] + bounds[0]) / 2.0, (bounds[3] + bounds[2]) / 2.0,
  //   (bounds[5] + bounds[4]) / 2.0 };
  double origin[3] = { bounds[0] + P * (bounds[1] - bounds[0]), (bounds[3] + bounds[2]) / 2.0,
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
