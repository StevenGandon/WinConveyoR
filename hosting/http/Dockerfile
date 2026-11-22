FROM debian:bookworm-slim

RUN apt-get update && \
    apt-get install -y sudo && \
    apt-get install -y w3m && \
    apt-get install -y apache2 && \
    a2enmod rewrite && \
    a2enmod socache_shmcb && \
    apt-get autoremove && \
    apt-get clean && \
    apt-get autoclean

RUN rm -rf /var/www/html
RUN ln -s /mnt/mirror /var/www/html

RUN echo "*               soft    nofile          8192" >> /etc/security/limits.conf
RUN echo "*               hard    nofile          8192" >> /etc/security/limits.conf

COPY ./apache2.conf /etc/apache2/apache2.conf

STOPSIGNAL SIGWINCH

EXPOSE 80

CMD sudo apache2ctl -D FOREGROUND
