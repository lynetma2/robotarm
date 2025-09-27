FROM python:3.11-slim
LABEL authors="99sun"

RUN apt update
RUN apt install -y gcc
RUN apt install -y libhidapi-dev
RUN cp 70-ps5-controller.rules /etc/udev/rules.d
RUN udevadm control --reload-rules
RUN udevadm trigger

WORKDIR /usr/src/app

COPY . .
RUN pip install --no-cache-dir --upgrade pip \
 && pip install --no-cache-dir -r requirements.txt

CMD ["python", "./dualsenseTest.py"]
