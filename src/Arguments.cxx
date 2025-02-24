//
// Created by spiros.tsalikis on 9/24/24.
//

#include "Arguments.h"
#include "CLI/CLI.hpp"
#include "vtkm/cont/DeviceAdapterTag.h"
#include "vtkm/cont/RuntimeDeviceTracker.h"

#include <thread>

namespace
{
bool DeviceIsAvailable(vtkm::cont::DeviceAdapterId id)
{
  if (id == vtkm::cont::DeviceAdapterTagAny{})
  {
    return true;
  }

  if (id.GetValue() <= 0 || id.GetValue() >= VTKM_MAX_DEVICE_ADAPTER_ID ||
    id == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    return false;
  }

  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  bool result = false;
  try
  {
    result = tracker.CanRunOn(id);
  }
  catch (...)
  {
    result = false;
  }
  return result;
}

std::string GetValidDeviceNames()
{
  std::ostringstream names;
  names << "\"Any\" ";

  for (vtkm::Int8 i = 0; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    auto id = vtkm::cont::make_DeviceAdapterId(i);
    if (DeviceIsAvailable(id))
    {
      names << "\"" << id.GetName() << "\" ";
    }
  }
  return names.str();
}
}

void Arguments::ParseArguments(int argc, char** argv)
{
  std::unique_ptr<CLI::App> app = std::make_unique<CLI::App>("Clip Evaluation");

  app->add_option("-i,--input", this->InputFileName, "Input file name")->required();

  app->add_option("-d,--device", this->DeviceName,
    "Device name. Available: " + ::GetValidDeviceNames() + ". (Default: TBB).");

  app->add_option("-t,--threads", this->NumberOfThreads, "Number of threads (Default: 1)")
    ->check(CLI::Range(1u, std::thread::hardware_concurrency()));

  app->add_option("-p,--percentage", this->Percentage, "Percentage")->check(CLI::Range(0.0, 1.0));

  app->add_option("-b,--batch-size", this->BatchSize, "Batch size (Default: 1000)");

  app->add_option("-n,--trials", this->NumberOfTrials, "Number of trials (Default: 1)");

  app->add_flag("--s-clip", this->SClip, "Run the S-Clip algorithm");

  app->add_flag("--p-batch-clip", this->PBatchClip, "Run the P-Batch-Clip algorithm");

  app->add_flag("--dp-clip", this->DPClip, "Run the DP-Clip algorithm");

  app->add_flag("--dp-batch-clip", this->DPBatchClip, "Run the DP-Batch-Clip algorithm");

  try
  {
    app->parse(argc, argv);
  }
  catch (const CLI::CallForHelp& e)
  {
    std::cout << app->help();
    exit(1);
  }
  catch (const CLI::CallForAllHelp& e)
  {
    std::cout << app->help();
    exit(1);
  }
  catch (const CLI::ParseError& e)
  {
    app->exit(e);
    exit(1);
  }
}
