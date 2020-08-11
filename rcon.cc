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

#include "rcon.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "absl/memory/memory.h"
#include "absl/random/random.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "packet.emb.h"

namespace minecraft_rcon {
namespace {

constexpr std::size_t kMaxPacketSize = 4096;

class MinecraftRconImpl : public MinecraftRcon {
 public:
  MinecraftRconImpl(absl::string_view address, absl::string_view password) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo *info = nullptr;
    std::pair<std::string, std::string> host_and_port =
        absl::StrSplit(address, ":");
    ABSL_RAW_CHECK(
        getaddrinfo(host_and_port.first.c_str(), host_and_port.second.c_str(),
                    &hints, &info) == 0,
        "");

    for (addrinfo *p = info; p != nullptr; p = p->ai_next) {
      fd_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (fd_ == -1) continue;

      if (connect(fd_, p->ai_addr, p->ai_addrlen) == -1) {
        close(fd_);
        continue;
      } else {
        break;
      }
    }
    ABSL_RAW_CHECK(fd_ != -1, "");
    freeaddrinfo(info);

    std::cout << "Connected to " << address << std::endl;
  }

  ~MinecraftRconImpl() override {
    if (fd_ != -1) close(fd_);
  }

  std::string Send(PacketType packet_type, absl::string_view body) override {
    const auto id =
        absl::Uniform<int32_t>(bitgen_, std::numeric_limits<int32_t>::lowest(),
                               std::numeric_limits<int32_t>::max());

    auto request_buf = std::vector<uint8_t>(body.size() + 14);
    auto request = MakePacketView(&request_buf);
    request.size().Write(request_buf.size() - 4);
    request.id().Write(id);
    request.packet_type().Write(packet_type);
    body.copy(reinterpret_cast<char *>(request.body().BackingStorage().data()),
              body.size());
    write(fd_, request_buf.data(), request_buf.size());
    std::cout << ::emboss::WriteToString(request) << std::endl;

    auto response_buf = std::vector<uint8_t>(kMaxPacketSize);
    recv(fd_, response_buf.data(), response_buf.size(), 0);
    auto response = MakePacketView(&response_buf);
    std::cout << ::emboss::WriteToString(response) << std::endl;
    return "";
  }

 private:
  int fd_ = -1;
  absl::BitGen bitgen_;
};
}  // namespace

std::unique_ptr<MinecraftRcon> MinecraftRcon::New(absl::string_view address,
                                                  absl::string_view password) {
  return absl::make_unique<MinecraftRconImpl>(address, password);
}

}  // namespace minecraft_rcon
