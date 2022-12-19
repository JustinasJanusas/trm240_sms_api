# trm240_sms_api
trm240 api used to send and read messages, to send commands use ZeroMQ library.

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

## Command syntax
Commands are sent in json format, there are 3 types of commands. Command type is declared in "method" variable. Different type commands require different variables. Command types are:
- "send" - used to send messages. Uses "phone" and "message" variables to specify message receiver and text which should be sent (**WARNING: command doesn't work with longer texts because they do not fit in a single message (possible fix in the future)**). Example: 
```
python3 client.py '{"method":"send", "phone":"+37064745286", "message":"žinutė"}'
```
- "read" - used to read received messages. Uses "type" argument to declare which messages should be read. There are three types: "all", "read" and "unread". There might be some parsing errors with unknown message formats. Example:
```
python3 client.py '{"method":"read", "type":"all"}'
```
- "custom" - used to write a single command to serial. Uses "command" argument, which will be written to serial (**NOTE: you don't have to add '\r' at the end, program will do it automatically**). Example:
```
python3 client.py '{"method":"custom", "command":"AT+CMGF=1"}'
```
