////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#include "ApplicationFeatures/TempFeature.h"

#include "Basics/ArangoGlobalContext.h"
#include "Basics/files.h"
#include "Basics/FileUtils.h"
#include "Logger/Logger.h"
#include "ProgramOptions/ProgramOptions.h"
#include "ProgramOptions/Section.h"

using namespace arangodb;
using namespace arangodb::options;

TempFeature::TempFeature(application_features::ApplicationServer* server,
                         std::string const& appname)
    : ApplicationFeature(server, "Temp"), _path(), _appname(appname) {
  setOptional(false);
  requiresElevatedPrivileges(false);
  startsAfter("Logger");
  startsAfter("Random");
}

void TempFeature::collectOptions(std::shared_ptr<ProgramOptions> options) {
  options->addOldOption("temp-path", "temp.path");

  options->addSection("temp", "Configure the temporary files");

  options->addOption("--temp.path", "path for temporary files",
                     new StringParameter(&_path));
}

void TempFeature::validateOptions(std::shared_ptr<ProgramOptions> options) {
  if (_path.length() > 0 ) {
    basics::FileUtils::makePathAbsolute(_path);
  }
}

void TempFeature::prepare() {
  TRI_SetApplicationName(_appname.c_str());
}

void TempFeature::start() {
  if (!_path.empty()) {
    TRI_SetUserTempPath((char*)_path.c_str());
  }

  // must be used after drop privileges and be called to set it to avoid raise
  // conditions
  TRI_GetTempPath();

  // signal that the temp path is available
  auto context = ArangoGlobalContext::CONTEXT;

  if (context != nullptr) {
    context->createMiniDumpFilename();
  }
}
