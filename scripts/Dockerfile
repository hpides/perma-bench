# CLion remote docker environment (How to build docker container, run and stop it)
# Taken and modified from: https://blog.jetbrains.com/clion/2020/01/using-docker-with-clion
#
# Build and run:
#   Check out the run_docker.sh file or the link above for details.
#
# ssh credentials (test user):
#   user@password

FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y \
    ssh build-essential gcc g++ gdb clang cmake rsync tar python git bash-completion \
    autoconf pkg-config pandoc zlib1g-dev \
  && apt-get clean

RUN apt-get install -y \
    libndctl-dev libdaxctl-dev libnuma-dev libmemkind-dev

RUN git clone https://github.com/pmem/pmdk && \
    cd pmdk && \
    git checkout tags/1.10 && \
    make install -j && \
    ldconfig

RUN ( \
    echo 'LogLevel DEBUG2'; \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
  ) > /etc/ssh/sshd_config_remote_pmem \
  && mkdir /run/sshd

RUN useradd -m user \
  && yes password | passwd user

CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_remote_pmem"]
