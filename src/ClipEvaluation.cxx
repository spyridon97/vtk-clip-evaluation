#include <viskores/Version.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/FieldSelection.h>
#include <viskores/filter/MapFieldPermutation.h>

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
  YamlWriter& log, bool firstRun = false) -> viskores::Float64
{
  vtkNew<ClipAlgorithm> clip;
  clip->SetInputData(inData);
  clip->SetInsideOut(false);
  clip->SetValue(0.0);
  clip->SetClipFunction(function);
  clip->SetBatchSize(batchSize);
  clip->Modified();

  viskores::cont::Timer timer;
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
  viskores::Float64 elapsedTime = timer.GetElapsedTime();
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
VISKORES_CONT bool DoMapField(
  viskores::cont::DataSet& result, const viskores::cont::Field& field, ClipWorklet& worklet)
{
  if (field.IsPointField())
  {
    viskores::cont::UnknownArrayHandle inputArray = field.GetData();
    viskores::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();

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
    viskores::cont::ArrayHandle<viskores::Id> permutation = worklet.GetCellMapOutputToInput();
    return viskores::filter::MapFieldPermutation(field, permutation, result);
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
VISKORES_CONT void MapFieldsOntoOutput(const viskores::cont::DataSet& input,
  const viskores::filter::FieldSelection& fieldSelection, viskores::cont::DataSet& output,
  FieldMapper&& fieldMapper)
{
  // Basic field mapping
  for (viskores::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
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

  for (viskores::IdComponent csIndex = 0; csIndex < input.GetNumberOfCoordinateSystems(); ++csIndex)
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
auto RunViskoresTrial(const viskores::cont::DataSet& inData, viskores::ImplicitFunctionGeneral function,
  unsigned int batchSize, YamlWriter& log, bool firstRun = false) -> viskores::Float64
{
  const viskores::cont::UnknownCellSet& unknownCellSet = inData.GetCellSet();
  const auto inCellSet = unknownCellSet.ResetCellSetList<VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED>();
  const viskores::cont::CoordinateSystem& inCoords = inData.GetCoordinateSystem(0);

  viskores::cont::CellSetExplicit<> outCellSet;

  std::stringstream dummyStream;
  YamlWriter dummyLog(dummyStream);

  ClipWorklet clip;

  viskores::cont::Timer timer;
  timer.Start();
  try
  {
    outCellSet = clip.Run(
      inCellSet, function, 0, inCoords, batchSize, firstRun ? dummyLog : log, false /*inverse*/);
  }
  catch (viskores::cont::Error& e)
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
  viskores::Float64 elapsedTime = timer.GetElapsedTime();
  if (!firstRun)
  {
    log.AddDictionaryEntry("seconds-clip", elapsedTime);
  }
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, clip); };
  viskores::cont::DataSet outDataSet;
  outDataSet.SetCellSet(outCellSet);
  timer.Start();
  MapFieldsOntoOutput(inData, viskores::filter::FieldSelection::Mode::All, outDataSet, mapper);
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
auto DoViskoresRun(const std::string& algorithmName, unsigned int numTrials,
  const viskores::cont::DataSet& inData, viskores::ImplicitFunctionGeneral& function,
  unsigned int& batchSize, YamlWriter& log) -> void
{
  log.StartListItem();
  log.AddDictionaryEntry("algorithm-name", algorithmName);
  log.AddDictionaryEntry("batch-size", batchSize);

  log.AddDictionaryEntry(
    "first-run-time", RunViskoresTrial<ClipWorklet>(inData, function, batchSize, log, true));

  if (numTrials > 0)
  {
    log.StartBlock("trials");
    for (unsigned int trial = 0; trial < numTrials; trial++)
    {
      log.StartListItem();
      log.AddDictionaryEntry("trial-index", trial);
      log.AddDictionaryEntry(
        "seconds-total", RunViskoresTrial<ClipWorklet>(inData, function, batchSize, log));
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
    args.DeviceName != "TBB" ? viskores::cont::make_DeviceAdapterId(args.DeviceName).GetName() : "TBB";

  YamlWriter log;
  log.StartListItem();

  log.AddDictionaryEntry("vtk-version", VTK_VERSION_FULL);
  log.AddDictionaryEntry("viskores-version", VISKORES_VERSION_FULL);
  log.AddDictionaryEntry("hostname", sysinfo.GetHostname());
  std::time_t currentTime = std::time(nullptr);
  char timeString[256];
  std::strftime(timeString, 256, "%Y-%m-%dT%H:%M:%S%z", std::localtime(&currentTime));
  log.AddDictionaryEntry("date", timeString);

  vtkSMPTools::Initialize(static_cast<int>(args.NumberOfThreads));
  // Construct the command line string for viskores::cont::Initialize
  std::vector<std::string> strings = { argv[0], "--viskores-device", deviceName };
  strings.emplace_back("--viskores-num-threads");
  strings.push_back(std::to_string(args.NumberOfThreads));
  std::vector<char*> argvVector;
  for (const auto& str : strings)
  {
    argvVector.push_back(const_cast<char*>(str.c_str()));
  }
  argvVector.push_back(nullptr);
  int Viskores_argc = static_cast<int>(argvVector.size() - 1);
  char** Viskores_argv = argvVector.data();
  auto result = viskores::cont::Initialize(Viskores_argc, Viskores_argv,
    viskores::cont::InitializeOptions::RequireDevice | viskores::cont::InitializeOptions::ErrorOnBadOption);
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
  // viskores::cont::DataSet ViskoresInputData;
  // if (args.PHashSort || args.PHashFight || args.PHashCount)
  // {
  //   ViskoresInputData = tovtkm::Convert(vtkInputData, tovtkm::FieldsFlag::PointsAndCells);
  // }
  viskores::cont::DataSet ViskoresInputData =
    tovtkm::Convert(vtkInputData, tovtkm::FieldsFlag::PointsAndCells);
  // deallocate the VTK data if it is not needed
  // if (!(args.HashDistribution || args.SClassifier || args.SHash || args.PClassifier ||
  // args.PHash))
  // {
  //   vtkInputData = nullptr;
  // }
  tovtkm::ImplicitFunctionConverter clipFunctionConverter;
  clipFunctionConverter.Set(vtkClipFunction);
  auto ViskoresClipFunction = clipFunctionConverter.Get();

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
    DoViskoresRun<viskores::worklet::ClipDPClip>(
      "DP-Clip", args.NumberOfTrials, ViskoresInputData, ViskoresClipFunction, args.BatchSize, log);
  }
  if (args.DPBatchClip)
  {
    DoViskoresRun<viskores::worklet::ClipDPBatchClip>(
      "DP-Batch-Clip", args.NumberOfTrials, ViskoresInputData, ViskoresClipFunction, args.BatchSize, log);
  }
  log.EndBlock();
}