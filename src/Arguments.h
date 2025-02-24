//
// Created by spiros.tsalikis on 9/24/24.
//

#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>

struct Arguments
{
  std::string InputFileName;
  unsigned int NumberOfThreads = 1;
  std::string DeviceName = "TBB";
  unsigned int NumberOfTrials = 1;
  double Percentage = 0.5;

  unsigned int BatchSize;

  bool SClip = false;
  bool PBatchClip = false;
  bool DPClip = false;
  bool DPBatchClip = false;

  /**
   * @brief Parse command line arguments.
   *
   * @return The command line arguments
   */
  void ParseArguments(int argc, char** argv);
};

#endif // CLI_H
