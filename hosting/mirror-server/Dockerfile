FROM debian:bookworm-slim

RUN apt-get update && \
    apt-get install -y sudo && \
    apt-get install -y python3 && \
    apt-get install -y pip && \
    apt-get autoremove && \
    apt-get clean && \
    apt-get autoclean

RUN mkdir /var/wcr
COPY . /var/wcr

RUN pip install -r /var/wcr/requirements.txt --break-system-packages

RUN chmod +x /var/wcr/wcmgr.py && ln -sn /var/wcr/wcmgr.py /bin/wcmgr
RUN chmod +x /var/wcr/wcrmirror.py && ln -sn /var/wcr/wcrmirror.py /bin/wcrmirror
RUN chmod +x /var/wcr/wcrpackager.py && ln -sn /var/wcr/wcrpackager.py /bin/wcrpackager

EXPOSE 1674

CMD ["/bin/wcrmirror"]