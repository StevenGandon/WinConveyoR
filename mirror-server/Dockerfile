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

RUN chmod +x /var/wcr/wcrmgr.py && ln -sn /var/wcr/wcrmgr.py /bin/wcrmgr
RUN chmod +x /var/wcr/wcrmirror.py && ln -sn /var/wcr/wcrmirror.py /bin/wcrmirror
RUN chmod +x /var/wcr/wcrpackager.py && ln -sn /var/wcr/wcrpackager.py /bin/wcrpackager

EXPOSE 1674

CMD ["python3", "/var/wcr/wcrmirror.py"]