/*===================================================================
mitkPhotoacousticBeamformingFilter
The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "mitkProperties.h"
#include "mitkImageReadAccessor.h"
#include <algorithm>
#include <itkImageIOBase.h>
#include <chrono>
#include <thread>
#include <itkImageIOBase.h>
#include "mitkImageCast.h"
#include "mitkBeamformingUtils.h"

mitk::BeamformingUtils::BeamformingUtils()
{
}

mitk::BeamformingUtils::~BeamformingUtils()
{
}

float* mitk::BeamformingUtils::VonHannFunction(int samples)
{
  float* ApodWindow = new float[samples];

  for (int n = 0; n < samples; ++n)
  {
    ApodWindow[n] = (1 - cos(2 * itk::Math::pi * n / (samples - 1))) / 2;
  }

  return ApodWindow;
}

float* mitk::BeamformingUtils::HammFunction(int samples)
{
  float* ApodWindow = new float[samples];

  for (int n = 0; n < samples; ++n)
  {
    ApodWindow[n] = 0.54 - 0.46*cos(2 * itk::Math::pi*n / (samples - 1));
  }

  return ApodWindow;
}

float* mitk::BeamformingUtils::BoxFunction(int samples)
{
  float* ApodWindow = new float[samples];

  for (int n = 0; n < samples; ++n)
  {
    ApodWindow[n] = 1;
  }

  return ApodWindow;
}

void mitk::BeamformingUtils::DASSphericalLine(
  float* input, float* output, float inputDim[2], float outputDim[2],
  const short& line, const mitk::BeamformingSettings::Pointer config)
{
  const float* apodisation = config->GetApodizationFunction();
  const short apodArraySize = config->GetApodizationArraySize();
  MITK_INFO << "=============== apodisation = " << apodisation;
  MITK_INFO << "=============== apodArraySize = " << apodArraySize;
  const float* elementHeights = config->GetElementHeights();
  const float* elementPositions = config->GetElementPositions();

  float& inputS = inputDim[1];
  float& inputL = inputDim[0];

  float& outputS = outputDim[1];
  float& outputL = outputDim[0];

  short AddSample = 0;
  float l_p = 0;
  float s_i = 0;

  short usedLines = inputL;
  float apod_mult = 1;

  float totalSamples_i = (float)(config->GetReconstructionDepth()) / (float)(config->GetSpeedOfSound() * config->GetTimeSpacing());
  totalSamples_i = totalSamples_i <= inputS ? totalSamples_i : inputS;

  l_p = (float)line / outputL * config->GetHorizontalExtent();

  for (short sample = 0; sample < outputS; ++sample)
  {
    s_i = (float)sample / outputS * totalSamples_i;
    usedLines = inputL;
    apod_mult = (float)apodArraySize / (float)usedLines;

    for (short l_s = 0; l_s < inputL; ++l_s)
    {
      AddSample = (int)sqrt(
        pow(s_i-elementHeights[l_s]/(config->GetSpeedOfSound()*config->GetTimeSpacing()), 2)
        +
        pow((1 / (config->GetTimeSpacing()*config->GetSpeedOfSound())) * (l_p - elementPositions[l_s]), 2)
      ) + (1 - config->GetIsPhotoacousticImage())*s_i;
      if (AddSample < inputS && AddSample >= 0)
        output[sample*(short)outputL + line] += input[l_s + AddSample*(short)inputL] *
        apodisation[(short)((l_s)*apod_mult)];
      else
        --usedLines;
    }
    output[sample*(short)outputL + line] = output[sample*(short)outputL + line] / usedLines;
  }
}

void mitk::BeamformingUtils::DMASSphericalLine(
  float* input, float* output, float inputDim[2], float outputDim[2],
  const short& line, const mitk::BeamformingSettings::Pointer config)
{
  const float* apodisation = config->GetApodizationFunction();
  const short apodArraySize = config->GetApodizationArraySize();

  const float* elementHeights = config->GetElementHeights();
  const float* elementPositions = config->GetElementPositions();

  float& inputS = inputDim[1];
  float& inputL = inputDim[0];

  float& outputS = outputDim[1];
  float& outputL = outputDim[0];

  float l_p = 0;
  float s_i = 0;

  float apod_mult = 1;
  float mult = 0;

  short usedLines = inputL;

  float totalSamples_i = (float)(config->GetReconstructionDepth()) /
    (float)(config->GetSpeedOfSound() * config->GetTimeSpacing());
  totalSamples_i = totalSamples_i <= inputS ? totalSamples_i : inputS;

  l_p = (float)line / outputL * config->GetHorizontalExtent();

  for (short sample = 0; sample < outputS; ++sample)
  {
    s_i = (float)sample / outputS * totalSamples_i;
    usedLines = inputL;
    apod_mult = (float)apodArraySize / (float)usedLines;

    //calculate the AddSamples beforehand to save some time
    short* AddSample = new short[usedLines];
    for (short l_s = 0; l_s < usedLines; ++l_s)
    {
      AddSample[l_s] = (int)sqrt(
        pow(s_i - elementHeights[l_s] / (config->GetSpeedOfSound()*config->GetTimeSpacing()), 2)
        +
        pow((1 / (config->GetTimeSpacing()*config->GetSpeedOfSound())) * (l_p - elementPositions[l_s]), 2)
      ) + (1 - config->GetIsPhotoacousticImage())*s_i;
    }

    float s_1 = 0;
    float s_2 = 0;

    for (short l_s1 = 0; l_s1 < usedLines - 1; ++l_s1)
    {
      if (AddSample[l_s1] < inputS && AddSample[l_s1] >= 0)
      {
        for (short l_s2 = l_s1 + 1; l_s2 < inputL; ++l_s2)
        {
          if (AddSample[l_s2] < inputS && AddSample[l_s2] >= 0)
          {
            s_2 = input[l_s2 + AddSample[l_s2] * (short)inputL];
            s_1 = input[l_s1 + AddSample[l_s1] * (short)inputL];

            mult = s_2 * apodisation[(int)((l_s2)*apod_mult)] * s_1 * apodisation[(int)((l_s1)*apod_mult)];
            output[sample*(short)outputL + line] += sqrt(fabs(mult)) * ((mult > 0) - (mult < 0));
          }
        }
      }
      else
        --usedLines;
    }

    output[sample*(short)outputL + line] = output[sample*(short)outputL + line] / (float)(pow(usedLines, 2) - (usedLines - 1));

    delete[] AddSample;
  }
}

void mitk::BeamformingUtils::sDMASSphericalLine(
  float* input, float* output, float inputDim[2], float outputDim[2],
  const short& line, const mitk::BeamformingSettings::Pointer config)
{
  const float* apodisation = config->GetApodizationFunction();
  const short apodArraySize = config->GetApodizationArraySize();

  const float* elementHeights = config->GetElementHeights();
  const float* elementPositions = config->GetElementPositions();

  float& inputS = inputDim[1];
  float& inputL = inputDim[0];

  float& outputS = outputDim[1];
  float& outputL = outputDim[0];

  float l_p = 0;
  float s_i = 0;

  float apod_mult = 1;

  float mult = 0;

  short usedLines = inputL;

  float totalSamples_i = (float)(config->GetReconstructionDepth()) /
    (float)(config->GetSpeedOfSound() * config->GetTimeSpacing());
  totalSamples_i = totalSamples_i <= inputS ? totalSamples_i : inputS;

  l_p = (float)line / outputL * config->GetHorizontalExtent();

  for (short sample = 0; sample < outputS; ++sample)
  {
    s_i = (float)sample / outputS * totalSamples_i;
    usedLines = inputL;
    apod_mult = (float)apodArraySize / (float)usedLines;

    //calculate the AddSamples beforehand to save some time
    short* AddSample = new short[usedLines];
    for (short l_s = 0; l_s < usedLines; ++l_s)
    {
      AddSample[l_s] = (int)sqrt(
        pow(s_i - elementHeights[l_s] / (config->GetSpeedOfSound()*config->GetTimeSpacing()), 2)
        +
        pow((1 / (config->GetTimeSpacing()*config->GetSpeedOfSound())) * (l_p - elementPositions[l_s]), 2)
      ) + (1 - config->GetIsPhotoacousticImage())*s_i;
    }

    float s_1 = 0;
    float s_2 = 0;
    float sign = 0;

    for (short l_s1 = 0; l_s1 < inputL - 1; ++l_s1)
    {
      if (AddSample[l_s1] < inputS && AddSample[l_s1] >= 0)
      {
        s_1 = input[l_s1 + AddSample[l_s1] * (short)inputL];
        sign += s_1;

        for (short l_s2 = l_s1 + 1; l_s2 < inputL; ++l_s2)
        {
          if (AddSample[l_s2] < inputS && AddSample[l_s2] >= 0)
          {
            s_2 = input[l_s2 + AddSample[l_s2] * (short)inputL];

            mult = s_2 * apodisation[(int)((l_s2)*apod_mult)] * s_1 * apodisation[(int)((l_s1)*apod_mult)];
            output[sample*(short)outputL + line] += sqrt(fabs(mult)) * ((mult > 0) - (mult < 0));
          }
        }
      }
      else
        --usedLines;
    }

    output[sample*(short)outputL + line] = output[sample*(short)outputL + line] / (float)(pow(usedLines, 2) - (usedLines - 1)) * ((sign > 0) - (sign < 0));

    delete[] AddSample;
  }
}
