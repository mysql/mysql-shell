FROM "mysql:8.0.32-debian"
RUN apt-get update && apt-get install -y openssh-server
RUN mkdir /var/run/sshd
RUN echo 'root:root' | chpasswd
RUN useradd -ms /bin/bash testuser
RUN echo 'testuser:testuser' | chpasswd
RUN sed -i 's/#*PermitRootLogin prohibit-password/PermitRootLogin yes/g' /etc/ssh/sshd_config
RUN sed -i 's/#*Port 22/Port 2222/g' /etc/ssh/sshd_config
RUN sed -i 's/ChallengeResponseAuthentication no/ChallengeResponseAuthentication yes/g' /etc/ssh/sshd_config
RUN sed -i 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' /etc/pam.d/sshd
RUN mkdir /root/.ssh
RUN chmod 700 /root/.ssh
COPY authorized_keys.test /root/.ssh/authorized_keys
RUN chmod 600 /root/.ssh/authorized_keys

# We need to inject sshd into docker-entrypoint
RUN perl -i.original -pe 's/(\s*)_main "\$@"/$1me=`whoami`\n$1if [ "\$me" != "mysql" ]; then\n$1  echo "About to start sshd"\n$1  \/usr\/sbin\/sshd\n$1  status=\$?\n\n$1  if [ \$status -ne 0 ]; then\n$1    echo "Failed to start sshd: \$status"\n$1  else\n$1   echo "sshd started successfully!"\n$1  fi\n$1fi\n\n$1_main "\$@"/igs' /usr/local/bin/docker-entrypoint.sh

EXPOSE 2222
CMD ["mysqld"]
