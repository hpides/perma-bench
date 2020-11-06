# CLion remote docker environment (How to build docker container, run and stop it)
# Taken and modified from: https://blog.jetbrains.com/clion/2020/01/using-docker-with-clion
#
# Build and run:
#   docker build -t clion/remote-pmem scripts
#   docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 --name clion_remote_pmem clion/remote-pmem
#   ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2222"
#
# stop:
#   docker stop clion_remote_pmem
#
# ssh credentials (test user):
#   user@password

FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y \
      ssh build-essential gcc g++ gdb clang cmake rsync tar python git \
      autoconf pkg-config libndctl-dev libdaxctl-dev pandoc \
  && apt-get clean

RUN ( \
    echo 'LogLevel DEBUG2'; \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
  ) > /etc/ssh/sshd_config_test_clion \
  && mkdir /run/sshd

RUN useradd -m user \
  && yes password | passwd user

CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_test_clion"]
