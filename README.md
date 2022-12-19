# trm240_sms_api

## Dependencies
[libzmq](https://zeromq.org/download/) library, to install it on debian OS you can run this command:
```
apt-get install libzmq3-dev
```

## Build
Run "make" command in the root folder and the executable will be built in "build" folder

## Connect
To connect to trm240_sms_api you have to connect to tcp://localhost:5555, there is a client example in "client" folder, to use it just run it using python3 and give your command (explained below) as an argument, example:
```
python3 client.py '{"method":"send", "phone":"+37064325256", "message":"žinutė"}'
```
