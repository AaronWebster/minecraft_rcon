// Copyright 2020 Aaron Webster
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "packet.emb.h"
#include "rcon.h"

ABSL_FLAG(std::string, address, "", "RCON address, in the form host:port.");
ABSL_FLAG(std::string, password, "", "RCON password.");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  const std::string address = absl::GetFlag(FLAGS_address);
  ABSL_RAW_CHECK(!address.empty(), "Missing required --address.");

  const std::string password = absl::GetFlag(FLAGS_password);
  ABSL_RAW_CHECK(!password.empty(), "Missing required --password.");

  auto rcon = minecraft_rcon::MinecraftRcon::New(address, password);
  std::cout << rcon->Send(minecraft_rcon::PacketType::RCON_AUTHENTICATE,
                          password)
            << std::endl;
  std::cout << rcon->Send(minecraft_rcon::PacketType::RCON_EXEC_COMMAND, "list")
            << std::endl;
}
