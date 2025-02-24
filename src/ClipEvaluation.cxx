#include <vtkm/Version.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/FieldSelection.h>
#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkmlib/ArrayConverters.h>
#include <vtkmlib/CellSetConverters.h>
#include <vtkmlib/DataArrayConverters.h>
#include <vtkmlib/DataSetConverters.h>
#include <vtkmlib/ImplicitFunctionConverter.h>
#include <vtkmlib/UnstructuredGridConverter.h>

#include <vtkPlane.h>
#include <vtkSMPTools.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVersionFull.h>
#include <vtkXMLUnstructuredGridReader.h>

#include <vtksys/SystemInformation.hxx>

#include "ClipDPBatchClip.h"
#include "ClipDPClip.h"

#include "vtkTableBasedClipDataSetPBatchClip.h"
#include "vtkTableBasedClipDataSetSClip.h"

#include "Arguments.h"
#include "YamlWriter.h"

#include <ctime>
#include <sstream>
#include <vector>

auto ReadDataSet(const std::string& filename) -> vtkSmartPointer<vtkUnstructuredGrid>
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();
  return reader->GetOutput();
}

template <typename ClipAlgorithm>
auto RunVTKTrial(vtkUnstructuredGrid* inData, vtkImplicitFunction* function, unsigned int batchSize,
  YamlWriter& log, bool firstRun = false) -> vtkm::Float64
{
  vtkNew<ClipAlgorithm> clip;
  clip->SetInputData(inData);
  clip->SetInsideOut(false);
  clip->SetValue(0.0);
  clip->SetClipFunction(function);
  clip->SetBatchSize(batchSize);
  clip->Modified();

  vtkm::cont::Timer timer;
  timer.Start();
  try
  {
    clip->Update();
  }
  catch (std::exception& e)
  {
    log.AddDictionaryEntry("error", e.what());
    return 0.0;
  }
  auto outData = clip->GetOutput();
  timer.Stop();
  vtkm::Float64 elapsedTime = timer.GetElapsedTime();
  if (firstRun)
  {
    log.AddDictionaryEntry("num-output-points", outData->GetNumberOfPoints());
    log.AddDictionaryEntry("num-output-cells", outData->GetNumberOfCells());
  }
  return elapsedTime;
}

template <typename ClipAlgorithm>
auto DoVTKRun(const std::string& algorithmName, unsigned int numTrials, vtkUnstructuredGrid* inData,
  vtkImplicitFunction* function, unsigned int& batchSize, YamlWriter& log) -> void
{
  log.StartListItem();
  log.AddDictionaryEntry("algorithm-name", algorithmName);
  log.AddDictionaryEntry("batch-size", batchSize);

  log.AddDictionaryEntry(
    "first-run-time", RunVTKTrial<ClipAlgorithm>(inData, function, batchSize, log, true));

  if (numTrials > 0)
  {
    log.StartBlock("trials");
    for (unsigned int trial = 0; trial < numTrials; trial++)
    {
      log.StartListItem();
      log.AddDictionaryEntry("trial-index", trial);
      log.AddDictionaryEntry(
        "seconds-total", RunVTKTrial<ClipAlgorithm>(inData, function, batchSize, log));
    }
    log.EndBlock();
  }
}

template <typename ClipWorklet>
VTKM_CONT bool DoMapField(
  vtkm::cont::DataSet& result, const vtkm::cont::Field& field, ClipWorklet& worklet)
{
  if (field.IsPointField())
  {
    vtkm::cont::UnknownArrayHandle inputArray = field.GetData();
    vtkm::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();

    auto resolve = [&](const auto& concrete)
    {
      // use std::decay to remove const ref from the decltype of concrete.
      using BaseT = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      auto concreteOut = outputArray.ExtractArrayFromComponents<BaseT>();
      worklet.ProcessPointField(concrete, concreteOut);
    };

    inputArray.CastAndCallWithExtractedArray(resolve);
    result.AddPointField(field.GetName(), outputArray);
    return true;
  }
  else if (field.IsCellField())
  {
    // Use the precompiled field permutation function.
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetCellMapOutputToInput();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}

template <typename FieldMapper>
VTKM_CONT void MapFieldsOntoOutput(const vtkm::cont::DataSet& input,
  const vtkm::filter::FieldSelection& fieldSelection, vtkm::cont::DataSet& output,
  FieldMapper&& fieldMapper)
{
  // Basic field mapping
  for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (fieldSelection.IsFieldSelected(field))
    {
      fieldMapper(output, field);
    }
  }

  // Check if the ghost levels have been copied. If so, set so on the output.
  if (input.HasGhostCellField())
  {
    const std::string& ghostFieldName = input.GetGhostCellFieldName();
    if (output.HasCellField(ghostFieldName) && (output.GetGhostCellFieldName() != ghostFieldName))
    {
      output.SetGhostCellFieldName(ghostFieldName);
    }
  }

  for (vtkm::IdComponent csIndex = 0; csIndex < input.GetNumberOfCoordinateSystems(); ++csIndex)
  {
    auto coords = input.GetCoordinateSystem(csIndex);
    if (!output.HasCoordinateSystem(coords.GetName()))
    {
      if (!output.HasPointField(coords.GetName()))
      {
        fieldMapper(output, coords);
      }
      if (output.HasPointField(coords.GetName()))
      {
        output.AddCoordinateSystem(coords.GetName());
      }
    }
  }
}

template <typename ClipWorklet>
auto RunVTKmTrial(const vtkm::cont::DataSet& inData, vtkm::ImplicitFunctionGeneral function,
  unsigned int batchSize, YamlWriter& log, bool firstRun = false) -> vtkm::Float64
{
  const vtkm::cont::UnknownCellSet& unknownCellSet = inData.GetCellSet();
  const auto inCellSet = unknownCellSet.ResetCellSetList<VTKM_DEFAULT_CELL_SET_LIST_UNSTRUCTURED>();
  const vtkm::cont::CoordinateSystem& inCoords = inData.GetCoordinateSystem(0);

  vtkm::cont::CellSetExplicit<> outCellSet;

  std::stringstream dummyStream;
  YamlWriter dummyLog(dummyStream);

  ClipWorklet clip;

  vtkm::cont::Timer timer;
  timer.Start();
  try
  {
    outCellSet = clip.Run(
      inCellSet, function, 0, inCoords, batchSize, firstRun ? dummyLog : log, false /*inverse*/);
  }
  catch (vtkm::cont::Error& e)
  {
    log.AddDictionaryEntry("error", e.GetMessage());
    return 0.0;
  }
  catch (std::exception& e)
  {
    log.AddDictionaryEntry("error", e.what());
    return 0.0;
  }
  timer.Stop();
  vtkm::Float64 elapsedTime = timer.GetElapsedTime();
  if (!firstRun)
  {
    log.AddDictionaryEntry("seconds-clip", elapsedTime);
  }
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, clip); };
  vtkm::cont::DataSet outDataSet;
  outDataSet.SetCellSet(outCellSet);
  timer.Start();
  MapFieldsOntoOutput(inData, vtkm::filter::FieldSelection::Mode::All, outDataSet, mapper);
  timer.Stop();
  elapsedTime += timer.GetElapsedTime();
  if (firstRun)
  {
    log.AddDictionaryEntry(
      "num-output-points", outDataSet.GetCoordinateSystem().GetNumberOfPoints());
    log.AddDictionaryEntry("num-output-cells", outDataSet.GetNumberOfCells());
  }
  else
  {
    log.AddDictionaryEntry("seconds-mapping", timer.GetElapsedTime());
  }
  return elapsedTime;
}

template <typename ClipWorklet>
auto DoVTKmRun(const std::string& algorithmName, unsigned int numTrials,
  const vtkm::cont::DataSet& inData, vtkm::ImplicitFunctionGeneral& function,
  unsigned int& batchSize, YamlWriter& log) -> void
{
  log.StartListItem();
  log.AddDictionaryEntry("algorithm-name", algorithmName);
  log.AddDictionaryEntry("batch-size", batchSize);

  log.AddDictionaryEntry(
    "first-run-time", RunVTKmTrial<ClipWorklet>(inData, function, batchSize, log, true));

  if (numTrials > 0)
  {
    log.StartBlock("trials");
    for (unsigned int trial = 0; trial < numTrials; trial++)
    {
      log.StartListItem();
      log.AddDictionaryEntry("trial-index", trial);
      log.AddDictionaryEntry(
        "seconds-total", RunVTKmTrial<ClipWorklet>(inData, function, batchSize, log));
    }
    log.EndBlock();
  }
}

auto main(int argc, char** argv) -> int
{
  Arguments args;
  args.ParseArguments(argc, argv);

  vtksys::SystemInformation sysinfo;

  std::string deviceName =
    args.DeviceName != "TBB" ? vtkm::cont::make_DeviceAdapterId(args.DeviceName).GetName() : "TBB";

  YamlWriter log;
  log.StartListItem();

  log.AddDictionaryEntry("vtk-version", VTK_VERSION_FULL);
  log.AddDictionaryEntry("vtkm-version", VTKM_VERSION_FULL);
  log.AddDictionaryEntry("hostname", sysinfo.GetHostname());
  std::time_t currentTime = std::time(nullptr);
  char timeString[256];
  std::strftime(timeString, 256, "%Y-%m-%dT%H:%M:%S%z", std::localtime(&currentTime));
  log.AddDictionaryEntry("date", timeString);

  vtkSMPTools::Initialize(static_cast<int>(args.NumberOfThreads));
  // Construct the command line string for vtkm::cont::Initialize
  std::vector<std::string> strings = { argv[0], "--vtkm-device", deviceName };
  strings.emplace_back("--vtkm-num-threads");
  strings.push_back(std::to_string(args.NumberOfThreads));
  std::vector<char*> argvVector;
  for (const auto& str : strings)
  {
    argvVector.push_back(const_cast<char*>(str.c_str()));
  }
  argvVector.push_back(nullptr);
  int vtkm_argc = static_cast<int>(argvVector.size() - 1);
  char** vtkm_argv = argvVector.data();
  auto result = vtkm::cont::Initialize(vtkm_argc, vtkm_argv,
    vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::ErrorOnBadOption);
  log.AddDictionaryEntry("device", result.Device.GetName());
  log.AddDictionaryEntry("num-threads", args.NumberOfThreads);

  log.AddDictionaryEntry("input-file", args.InputFileName);

  vtkSmartPointer<vtkUnstructuredGrid> vtkInputData = ReadDataSet(args.InputFileName);

  log.AddDictionaryEntry("topology-connections", "regular");
  log.AddDictionaryEntry("num-input-points", vtkInputData->GetNumberOfPoints());
  log.AddDictionaryEntry("num-input-cells", vtkInputData->GetNumberOfCells());

  double bounds[6];
  vtkInputData->GetBounds(bounds);
  const double origin[3] = { bounds[0] + (1 - args.Percentage) * (bounds[1] - bounds[0]),
    (bounds[3] + bounds[2]) / 2.0, (bounds[5] + bounds[4]) / 2.0 };
  const double normal[3] = { 1.0, 0.0, 0.0 };
  vtkNew<vtkPlane> vtkClipFunction;
  vtkClipFunction->SetOrigin(origin);
  vtkClipFunction->SetNormal(normal);

  log.AddDictionaryEntry("percentage", args.Percentage);
  log.AddDictionaryEntry("clip-origin",
    std::to_string(origin[0]) + ", " + std::to_string(origin[1]) + ", " +
      std::to_string(origin[2]));
  log.AddDictionaryEntry("clip-normal",
    std::to_string(normal[0]) + ", " + std::to_string(normal[1]) + ", " +
      std::to_string(normal[2]));

  // Convert the VTK data to VTK-m data if needed
  // vtkm::cont::DataSet vtkmInputData;
  // if (args.PHashSort || args.PHashFight || args.PHashCount)
  // {
  //   vtkmInputData = tovtkm::Convert(vtkInputData, tovtkm::FieldsFlag::PointsAndCells);
  // }
  vtkm::cont::DataSet vtkmInputData =
    tovtkm::Convert(vtkInputData, tovtkm::FieldsFlag::PointsAndCells);
  // deallocate the VTK data if it is not needed
  // if (!(args.HashDistribution || args.SClassifier || args.SHash || args.PClassifier ||
  // args.PHash))
  // {
  //   vtkInputData = nullptr;
  // }
  tovtkm::ImplicitFunctionConverter clipFunctionConverter;
  clipFunctionConverter.Set(vtkClipFunction);
  auto vtkmClipFunction = clipFunctionConverter.Get();

  const auto datasetMemoryUsed = sysinfo.GetProcMemoryUsed();
  log.AddDictionaryEntry("dataset-memory-used", datasetMemoryUsed);

  log.StartBlock("experiments");

  if (args.SClip)
  {
    DoVTKRun<vtkTableBasedClipDataSetSClip>(
      "S-Clip", args.NumberOfTrials, vtkInputData, vtkClipFunction, args.BatchSize, log);
  }
  if (args.PBatchClip)
  {
    DoVTKRun<vtkTableBasedClipDataSetPBatchClip>(
      "P-Batch-Clip", args.NumberOfTrials, vtkInputData, vtkClipFunction, args.BatchSize, log);
  }
  if (args.DPClip)
  {
    DoVTKmRun<vtkm::worklet::ClipDPClip>(
      "DP-Clip", args.NumberOfTrials, vtkmInputData, vtkmClipFunction, args.BatchSize, log);
  }
  if (args.DPBatchClip)
  {
    DoVTKmRun<vtkm::worklet::ClipDPBatchClip>(
      "DP-Batch-Clip", args.NumberOfTrials, vtkmInputData, vtkmClipFunction, args.BatchSize, log);
  }
  log.EndBlock();
}