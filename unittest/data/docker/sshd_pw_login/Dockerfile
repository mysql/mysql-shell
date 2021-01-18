FROM "mysql:latest"
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
RUN head -1 /usr/local/bin/docker-entrypoint.sh > /docker-entrypoint.sh.tmp
RUN printf 'echo "About to start sshd"\n\
/usr/sbin/sshd\n\
status=$?\n\
if [ $status -ne 0 ]; then\n\
  echo "Failed to start sshd: $status"\n\
  exit $status\n\
fi\n\
echo "sshd started"\n\
' >> docker-entrypoint.sh.tmp

RUN tail -n +2 /usr/local/bin/docker-entrypoint.sh >> /docker-entrypoint.sh.tmp
RUN mv /docker-entrypoint.sh.tmp /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

EXPOSE 2222
CMD ["mysqld"]
