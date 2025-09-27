FROM python:3.11-slim
LABEL authors="99sun"

WORKDIR /usr/src/app
COPY . .

RUN apt update
RUN apt install -y gcc
RUN cp 70-ps5-controller.rules /etc/udev/rules.d
RUN udevadm control --reload-rules
RUN udevadm trigger
RUN apt install -y libhidapi-dev


RUN pip install --no-cache-dir --upgrade pip \
 && pip install --no-cache-dir -r requirements.txt

CMD ["python", "./dualsenseTest.py"]
