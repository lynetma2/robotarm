FROM python:3.11-slim
LABEL authors="99sun"

WORKDIR /usr/src/app

COPY . .
RUN pip install --no-cache-dir --upgrade pip \
    && pip install -no-cache-dir -r requirements.txt

CMD ["python", "./main.py"]