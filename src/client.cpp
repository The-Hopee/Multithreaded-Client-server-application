#include <iostream>
#include <thread>
#include <vector>
#include "hfa.h"
#include <cryptopp/cryptlib.h>
#include <cryptopp/rijndael.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/base64.h>

using namespace CryptoPP;

std::string key = "E9VwaE4nI8YElBMcdQE8guOWRc99d0cq";
std::string iv = "ZEkY3CvOENM1gu9xdb0t8fWQ2XPtZUgO";

std::string Encrypt(std::string str, std::string cipher, std::string iv) 
{ 
    //функция которая шифрует наш стринг/string с помощью уникальных ключей cipher и iv, возвращает зашифрованный текст(std::string).
    std::string Output;
    CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption Encryption((byte*)cipher.c_str(), cipher.length(), (byte*)iv.c_str());
    CryptoPP::StringSource Encryptor(str, true, new CryptoPP::StreamTransformationFilter(Encryption, new CryptoPP::Base64Encoder(new CryptoPP::StringSink(Output), false)));
    return Output;
}

std::string Decrypt(std::string str, std::string cipher, std::string iv) 
{ 
    //функция которая расшифрует зашифрованный стринг/string с помощью уникальных ключей cipher i iv, возвращает расшифрованный текст(std::string).
    std::string Output;
    CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption Decryption((byte*)cipher.c_str(), cipher.length(), (byte*)iv.c_str());
    CryptoPP::StringSource Decryptor(str, true, new CryptoPP::Base64Decoder(new CryptoPP::StreamTransformationFilter(Decryption, new CryptoPP::StringSink(Output))));
    return Output;
}


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
    if (msg_size <= 0)
    {
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
        if (msg_size <= 0)
        {
            std::cout << "Соединение с сервером разорвано." << std::endl;
            close(connect);
            return;
        }

        std::string msg = get_message(connect, msg_size);
        std::string decrypt_msg = Decrypt(msg,key,iv);
        if (decrypt_msg.empty()) 
        {
            std::cerr << "Ошибка при получении сообщения." << std::endl;
            close(connect);
            return;
        }

        std::cout << decrypt_msg << std::endl;

        if (decrypt_msg.compare("Вы покинули сервер") == 0) 
        {
            close(connect);
            return;
        }
    }
}

std::string keyGen() // Генератор рандомных ключей
{
    const int len = 32;
    std::string key;
    key.resize(len);
    std::string letters = "_@0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    int size_alpha = letters.size();

    for(int i = 0; i < len; i++)
    {
        key[i] = letters[rand()%size_alpha];
    }

    return key;
}

std::string passGen() // Генератор рандомных паролей
{
    const int len = 32;
    std::string password;
    std::string key = keyGen();
    password.resize(len);

    for(int i =0; i < len; i++)
    {
        password[i] = key[rand()%key.size()];
    }

    return password;
}

int checkPassword(std::string &pass)
{
    int count_spec_symb = 0;
    int count_high_lett = 0;
    int count_nums = 0;

    for(char c: pass)
    {
        if( isdigit(c) != 0 )
        {
            count_nums++;
        }
        else if( c >= 65 && c <= 90 )
        {
            count_high_lett++;
        }
        else if( c == '_' || c == '@' )
        {
            count_spec_symb++;
        }
    }

    if( count_high_lett > 0 && count_nums > 0 )
    {
        return 2;
    }
    else if( count_high_lett > 0 && count_nums > 0 && count_spec_symb > 0 )
    {
        return 1;
    }
    return 0;
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

int main(int argc,char* argv[])
{

    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    std::string command, answer,encrypt_command,decrypt_command;
    int msg_size = 0;

    std::cout << "Введите логин и пароль(Через пробел): ";
    getline(std::cin, command);

    std::vector<std::string> parse_com = Parse(command);

    if( parse_com.size() != 2 )
    {
        std::cerr << "Неверное количество параметров. Попробуйте позже.\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        return -1;
    }

    int connection = socket(AF_INET,SOCK_STREAM, 0);

    if(connect(connection,(sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "Ошибка подключения к серверу!" << std::endl;
        return -1;
    }

    std::cout << "Соединение установлено!" << std::endl;

    encrypt_command = Encrypt(command,key,iv);
    
    msg_size = encrypt_command.size();

    send(connection,(char*)&msg_size,sizeof(int),0);
    send(connection,encrypt_command.c_str(),encrypt_command.size(),0);

    answer = recieve_msg_to_autorizate(connection);
    decrypt_command = Decrypt(answer,key,iv);

    std::cout << decrypt_command << std::endl;

    if(decrypt_command.compare("Добро пожаловать в систему.") != 0)
    {
        getline(std::cin, command);

        encrypt_command = Encrypt(command,key,iv);

        msg_size = encrypt_command.size();
        send(connection,(char*)&msg_size,sizeof(int),0);
        send(connection,encrypt_command.c_str(),encrypt_command.size(),0);

        answer = recieve_msg_to_autorizate(connection);

        decrypt_command = Decrypt(answer,key,iv);

        std::cout << decrypt_command << std::endl;

        if( decrypt_command.compare("Добро пожаловать в систему.") != 0 )
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

        if( parse_com[0].compare("exit") == 0 || parse_com[0].compare("Exit") == 0 )
        {
            try
            {
                std::cout << "Отсоединение от сервера...\n";
                encrypt_command = Encrypt(command,key,iv);
                msg_size = encrypt_command.size();
                send(connection,(char*)&msg_size,sizeof(int),0);
                send(connection,encrypt_command.c_str(),encrypt_command.size(),0); 

                break;
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        else if( parse_com[0].compare("--c") == 0 )
        {
            std::string gen_pass = passGen();
            if( checkPassword(gen_pass) == 2 )
            {
                std::cout << "Пароль не слишком криптоскойкий, но подходит под условия эксплуатации" << std::endl;
                command.append(" ");
                command.append(gen_pass);
            }
            else if( checkPassword(gen_pass) == 1 )
            {
                std::cout << "Пароль криптостойкий" << std::endl;
                command.append(" ");
                command.append(gen_pass);
            }
            else
            {
                std::cout << "Пароль не криптостойкий" << std::endl;
                gen_pass = "";
            }
        }
        else if( parse_com[0].compare("-c") == 0 || parse_com[0].compare("--create") == 0 )
        {
            if(parse_com[3].size() < 8)
            {
                std::cerr << "Ошибка. Длина пароля меньше 8ми символов. Допустимая длина от 8 до 32 символов!" << std::endl;
            }
            else if( parse_com[3].size() > 32 )
            {
                std::cerr << "Ошибка. Длина пароля больше 32х символов. Допустимая длина от 8 до 32 символов!" << std::endl;
            }
        }

        encrypt_command = Encrypt(command,key,iv); //Шифруем команду. Эта переменная будет отправляться на сокет сервера

        msg_size = encrypt_command.size();

        send(connection,(char*)&msg_size,sizeof(int),0);
        send(connection,encrypt_command.c_str(),encrypt_command.size(),0);
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