#include <iostream>
#include <thread>
#include <vector>
#include "hfa.h"

int get_message_size(int connection) // собираем размер сообщения
{
    int size = 0;
    char* buffer_ptr = reinterpret_cast<char*>(&size);
    int total_received = 0;

    while (total_received < sizeof(int)) 
    {
        int bytes_received = recv(connection, buffer_ptr + total_received, sizeof(int) - total_received, 0);
        if (bytes_received <= 0) {
            return -1; // Ошибка или отключение клиента
        }
        total_received += bytes_received;
    }

    return size;
}

std::string get_message(int connection, int size) // собираем само сообщение
{
    std::string message;
    message.resize(size);

    char* buffer_ptr = &message[0];
    int total_received = 0;

    while (total_received < size) 
    {
        int bytes_received = recv(connection, buffer_ptr + total_received, size - total_received, 0);
        if (bytes_received <= 0) {
            return ""; // дисконект
        }
        total_received += bytes_received;
    }

    return message;
}

std::string recieve_msg_to_autorizate(int connect)
{
    int msg_size = get_message_size(connect);
    if (msg_size <= 0) {

        std::cout << "Соединение с сервером разорвано." << std::endl;
        close(connect);
        return "";
    }

    std::string msg = get_message(connect, msg_size);
    if (msg.empty()) 
    {
        std::cerr << "Ошибка при получении сообщения." << std::endl;
        close(connect);
        return "";
    }

    return msg;
}

void Recieve_msg(int connect)
{
    while (true) 
    {
        int msg_size = get_message_size(connect);
        if (msg_size <= 0) {

            std::cout << "Соединение с сервером разорвано." << std::endl;
            close(connect);
            return;
        }

        std::string msg = get_message(connect, msg_size);
        if (msg.empty()) 
        {
            std::cerr << "Ошибка при получении сообщения." << std::endl;
            close(connect);
            return;
        }

        std::cout << msg << std::endl;

        if (msg.compare("Вы покинули сервер") == 0) 
        {
            close(connect);
            return;
        }
    }
}

std::vector <std::string> Parse(std::string str_in) 
{
	std::string str_out = "";
	std::vector<std::string> vec;
	for (int i = 0; i < str_in.size(); i++)
	{
		if (str_in[i] != ' ') // собираем строчку пока не встретили " "
		{
			str_out.push_back(str_in[i]);
		}
		else // как только встретили " "
		{
			vec.push_back(str_out);
			str_out = "";
		}
	}
	vec.push_back(str_out);
	return vec;
}

// Сделать полноценное отсоединение клиента (протокол выхода с сохранением данных клиента, сохранением его логина id и пароля) 
// Если пользователь введет "нет" при ответе на такой сценарий. Нужно обработать такое событие
// Сервер падает при вводе только логина. Нужно исправить

int main(int argc,char* argv[])
{

    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    int connection = socket(AF_INET,SOCK_STREAM, 0);

    if(connect(connection,(sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "Ошибка подключения к серверу!" << std::endl;
        return -1;
    }

    std::cout << "Соединение установлено!" << std::endl;

    std::string command, answer;
    int msg_size = 0;

    std::vector<std::string> parse_com;

    std::cout << "Введите логин и пароль(Через пробел): ";
    getline(std::cin, command);
    
    msg_size = command.size();
    send(connection,(char*)&msg_size,sizeof(int),0);
    send(connection,command.c_str(),command.size(),0);

    answer = recieve_msg_to_autorizate(connection);

    std::cout << answer << std::endl;

    if( answer.compare("Неверное количество параметров. Попробуйте позже.") == 0)
    {
        close(connection);
        return -1;
    }

    if(answer.compare("Добро пожаловать в систему.") != 0)
    {
        getline(std::cin, command);

        msg_size = command.size();
        send(connection,(char*)&msg_size,sizeof(int),0);
        send(connection,command.c_str(),command.size(),0);

        answer = recieve_msg_to_autorizate(connection);

        std::cout << answer << std::endl;

        if( answer.compare("Добро пожаловать в систему.") != 0 )
        {
            close(connection);
            return -1;
        }
    }

    std::thread thr(Recieve_msg,connection);

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "Введите команду: ";
        getline(std::cin,command);
        parse_com = Parse(command);

        if(parse_com[0] == "exit" || parse_com[0] == "Exit")
        {
            try
            {
                std::cout << "Отсоединение от сервера...\n";
                msg_size = command.size();
                send(connection,(char*)&msg_size,sizeof(int),0);
                send(connection,command.c_str(),command.size(),0); 

                break;

            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        }

        msg_size = command.size();

        send(connection,(char*)&msg_size,sizeof(int),0);
        send(connection,command.c_str(),command.size(),0); 
    }

    try
    {
        if( thr.joinable() )
        {
            thr.join();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    close(connection); //Здесь клиент посылыет серверу команду для отсоединения

    return 0;
}