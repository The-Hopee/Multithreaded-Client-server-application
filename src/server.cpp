#include "hfa.h"
#include <thread>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <random>
#include <cryptopp/cryptlib.h>
#include <cryptopp/rijndael.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/base64.h>

using namespace CryptoPP;

#define MAX_CLIENTS 1024
#define ULL unsigned long long

std::vector<int> clients;
// нужно написать std::vector<int> autorizations;

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

std::string key = "E9VwaE4nI8YElBMcdQE8guOWRc99d0cq";
std::string iv = "ZEkY3CvOENM1gu9xdb0t8fWQ2XPtZUgO";

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

struct data
{
    std::string servis,login,password;
};

struct init
{
    std::string login,password;
    ULL id;
};

class Storage
{
private:
    std::unordered_map<ULL,std::vector<data>> storage;
    std::vector<init> init_data;
    std::vector<ULL> generated_ids;

    void init_admin_data()
    {
        std::ifstream fin("init.txt");
        //Реализовать чтение данных из файла init.txt

        if( !fin.is_open() )
        {
            throw std::string("Ошибка открытия файла init.txt");
        }

        std::string val1,val2;
        ULL val3;

        while( fin >> val1 >> val2 >> val3 )
        {
            std::string decrypt_login = Decrypt(val1, key, iv);
            std::string decrypt_password = Decrypt(val2, key, iv);
            init_data.push_back({decrypt_login,decrypt_password,val3});
        }
    }

    void init_id()
    {
        std::ifstream fin("generated_ids.txt");
        //Реализовать чтение данных из файла init.txt

        if( !fin.is_open() )
        {
            throw std::string("Ошибка открытия файла generated_ids.txt");
        }

        ULL val1;

        while( fin >> val1  )
        {
            generated_ids.push_back(val1);
        }

    }

    void distruct_ids()
    {
        std::ofstream fout("generated_ids.txt");

        if( !fout.is_open() )
        {
            throw std::string("Ошибка открытия файла generated_ids.txt");
        }

        for(auto it: generated_ids)
        {
            fout << it << "\n";
        }
    }

    void distruct()
    {
        std::ofstream fout("init.txt");

        if( !fout.is_open() )
        {
            throw std::string("Ошибка открытия файла init.txt");
        }

        for(auto it: init_data)
        {
            std::string encrypt_login = Encrypt(it.login,key,iv);
            std::string encrypt_password = Encrypt(it.password,key,iv);
            fout << encrypt_login << "\t" << encrypt_password << "\t" <<  it.id << "\n";
        }
    }

    ULL generate_id()
    {
        return rand() % 1'000'000;
    }

public:

    Storage()
    {
        try
        {
            init_admin_data();
            init_id();
        }
        catch(const std::string& e)
        {
            std::cerr << e << '\n';
        }
    }

    std::string Print(ULL id)
    {
        std::string msg;
        for(auto it: storage[id])
        {
            msg += "Сервис: " + it.servis + "\n";
            msg += "Логин: " + it.login + "\n";
            msg += "Пароль: " + it.password + "\n";
            msg += "\n";
        }

        return msg;
    }

    std::string Print(ULL id, std::string serv)
    {
        std::string msg;
        for(auto it: storage[id])
        {
            if( it.servis == serv)
            {
                msg += "Логин: " + it.login + "\n";
                msg += "Пароль: " + it.password + "\n";
                msg += "\n";
            }
        }

        return msg;
    }

    std::string Print(ULL id, std::string serv, std::string log)
    {
        for(auto it: storage[id])
        {
            if( it.servis == serv && it.login == log)
            {
                return "Пароль: " + it.password + "\n";
            }
        }

        return "Записи для данного сервиса и логина не существует\n";
    }

    std::string info()
    {
        std::string msg = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
        msg += "- -h/--help. Выводит справку о доступных командах                                                                                                                          -\n";
        msg += "- Exit/exit. Производит отключение клиента от сервера и последующую запись его данных на диск.                                                                             -\n";
        msg += "- -c/--create servis,login,pass Создает запись в хранилище по определенному сервису и логину. Если запись есть, то сообщает об этом.                                       -\n";
        msg += "- --c servis,login Создает запись в хранилище по определенному сервису и логину. Только здесь генерация пароля автоматическая Если запись есть, то сообщает об этом.       -\n";
        msg += "- -f/--find servis,login. Ищет запись в хранилище по определенному сервису и логину. Если записи нет, то сообщает об этом.                                                 -\n";
        msg += "- -d/--delete servis,login. Удаляет запись в хранилище по определенному сервису и логину. Если записи нет, то сообщает об этом.                                            -\n";
        msg += "- -e/--edit servis,login,newPass. Редактирует запись в хранилище по определенному сервису и логину и заменяет старый пароль на новый. Если записи нет, то сообщает об этом.-\n";
        msg += "- -p/--print servis,login/servis. Отображает запись по сервису, по сервису и логину. Если записи нет, то сообщает об этом.                                                 -\n";
        msg += "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
        return msg;
    }

    std::string enter_user(ULL id) //  Для считывания данных юзера из определенного файла
    {
        std::string name = "data_user_" + std::to_string(id) + ".txt";
        std::ifstream fin(name);

        if( !fin.is_open() )
        {
            return "Ошибка открытия файла " + name + "\n";
        }

        std::string val1,val2,val3;

        while( fin >> val1 >> val2 >> val3 )
        {
            std::string decrypt_servis = Decrypt(val1,key,iv);
            std::string decrypt_login = Decrypt(val2,key,iv);
            std::string decrypt_password = Decrypt(val3,key,iv);
            storage[id].push_back({decrypt_servis,decrypt_login,decrypt_password});
        }

        return "Данные пользователя " + name + " загружены\n";
    }

    std::string exit_user(ULL id) // Для записи данных юзера в определенный файл
    {
        std::string name = "data_user_" + std::to_string(id) + ".txt";
        std::ofstream fout(name);

        if( !fout.is_open() )
        {
            return "Ошибка открытия файла " + name + "\n"; 
        }

        for(auto it: storage[id])
        {
            std::string encrypt_servis = Encrypt(it.servis,key,iv);
            std::string encrypt_login = Encrypt(it.login,key,iv);
            std::string encrypt_password = Encrypt(it.password,key,iv);
            fout << encrypt_servis << "\t" << encrypt_login << "\t" << encrypt_password << "\n";
        }

        auto it = storage.find(id);

        if( it != storage.end() )
        {
            storage.erase(it);
        }

        fout.close();

        std::ofstream fout2("init.txt");

        if(!fout2.is_open())
        {
            return "Ошибка открытия файла init.txt\n";   
        }
        
        for(auto it: init_data)
        {
            if( it.id == id )
            {
                std::string encrypt_login = Encrypt(it.login,key,iv);
                std::string encrypt_password = Encrypt(it.password,key,iv);
                fout2 << encrypt_login << "\t" << encrypt_password << "\t" <<  it.id << "\n";
                break;
            }
        }

        fout2.close();

        std::ofstream fout3("generated_ids.txt");

        if(!fout3.is_open())
        {
            return "Ошибка открытия файла generated_ids.txt\n";   
        }    

        fout3 << id << "\n";

        fout3.close();

        return "Данные для " + name + " записаны\n";
    }

    ULL create_user(std::string log, std::string pass)
    {
        //1) нужно сгенерить рандомный id для мапы. Затем по этому id создать запись 
        ULL id = generate_id();
        
        //Проверка что сгенеренный id не существует. В случае если существует, рекурсивно вызывается функция

        while( std::count(generated_ids.begin(), generated_ids.end(), id) != 0 )
        {
            std::cout << "Повторная генерация id..." << std::endl;
            id = create_user(log,pass);
        }

        storage[id] = std::vector<data>(); // Создали ячейку юзера

        generated_ids.push_back(id); // Добавили его сгенеренный айди в хранилище айдишников

        init i;
        i.id = id;
        i.login = log;
        i.password = pass;
        init_data.push_back(i); // Добавили данные клиента в хранилище данных для входа

        return id;
    }

    std::string NewData(std::string serv, std::string log, std::string pass, ULL id)
    {
        auto it = std::find_if( storage[id].begin(), storage[id].end(),[&]( data& d)
        {
            return d.servis == serv && d.login == log && d.password == pass;
        });

        if( it == storage[id].end() ) //Значит такой записи еще нет
        {
            storage[id].push_back({serv,log,pass});
            return "Запись добавлена\n";
        }
        return "Запись уже существует\n";
    }

    std::string Find(ULL id, std::string serv, std::string log) //релизнуть несколько модов поиска  (Вся ячейка или какая-то конкретная запись или все конкретные записи по сервису)
    {
        auto it = std::find_if(storage[id].begin(), storage[id].end(),[&](data& d) 
        {
            return d.servis == serv && d.login == log;
        });

        if( it != storage[id].end() )
        {
            return "Данные для данного логина и сервиса есть\n";   
        }

        return "Данных для данного логина и сервиса нет\n";
    }

    std::string Find(ULL id, std::string serv) //релизнуть несколько модов поиска  (Вся ячейка или какая-то конкретная запись или все конкретные записи по сервису)
    {
        auto it = std::find_if(storage[id].begin(), storage[id].end(),[&](data& d) 
        {
            return d.servis == serv;
        });

        if( it != storage[id].end() )
        {
            return "Данные для данного логина и сервиса есть\n";   
        }

        return "Данных для данного логина и сервиса нет\n";
    }

    std::string Find(ULL id)
    {
        auto it = storage.find(id);

        if( it != storage.end() )
        {
            return "Данные для данного юзера есть\n";
        }

        return "Данных для данного юзера нет\n";
    }


    std::string Delete(ULL id, std::string serv, std::string log) // Релизнуть несколько способов удаления (Вся ячейка или какая-то конкретная запись или все конкретные записи по сервису)
    {
        auto it = std::find_if(storage[id].begin(), storage[id].end(),[&](data& d)
        {
            return d.servis == serv && d.login == log;
        });

        if( it != storage[id].end() )
        {
            storage[id].erase(it);
            return "Данные по данному сервису и логину удалены\n";
        }

        return "Данные по данному сервису и логину не найдены\n";
    }

    std::string Delete(ULL id)
    {
        auto it = storage.find(id);

        if( it != storage.end() )
        {
            storage.erase(id);
            return "Ячейка юзера по номеру " + std::to_string(id) + " удалена.\n";
        }

        return "Ячейка по номеру " + std::to_string(id) + " не обнаружена.\n";
    }

    std::string Edit(ULL id, std::string serv, std::string log, std::string pass) // Релизнуть редактирование записи по id и сервису с логином
    {
        auto it = std::find_if(storage[id].begin(), storage[id].end(),[&](data d)
        {
            return d.servis == serv && d.login == log;
        });

        if( it != storage[id].end() )
        {
            (*it).password = pass;
            return "Пароль для данного сервиса и логина изменен\n";
        }

        return "Пароль для данного сервиса и логина не изменен\n";
    }

    ULL autentificate(std::string log, std::string pass)
    {
        for(auto it: init_data)
        {
            if(it.login == log && it.password == pass)
            {
                return it.id;
            }
        }

        return -1;
    }

    ~Storage()
    {
        try
        {
            distruct();
            distruct_ids();
        }
        catch(const std::string& e)
        {
            std::cerr << e << '\n';
        }
        
    }

};

// код который считывает все пакеты размерности
int get_message_size(int socket) 
{
    int size = 0;
    char* buffer_ptr = reinterpret_cast<char*>(&size); // мы создаем указатель на начало переменной size в памяти и будем туда записывать битики
    int total_received = 0;

    while (total_received < sizeof(int)) // пока сумма принятого сообщения меньше размера инт переменной
    {
        int bytes_received = recv(socket, buffer_ptr + total_received, sizeof(int) - total_received, 0); // принимаем пакет данных
        if (bytes_received <= 0) // проверяем его на то что он не пустой
        {
            return -1; // и если пустой, значит клиент отсоединился
        }
        total_received += bytes_received;// добавляем к переменной-сумме значение принятых байтиков
    }

    return size; // и возвращаем ее размер
}

// Код который считывает все пакеты сообщений
std::string get_message(int socket, int size) 
{
    std::string message;
    message.resize(size); // перераспределяем размер

    char* buffer_ptr = &message[0]; // создаем указатель на место в памяти где расположена переменная message
    int total_received = 0;

    while (total_received < size)  // пока кол-во принятых байтиков меньше размера пакета
    {
        int bytes_received = recv(socket, buffer_ptr + total_received, size - total_received, 0); // принимаем байтики, добавляем их указателю на область памяти message, и вычитаем из общего размера принятый пакет
        if (bytes_received <= 0) // проверяем что пакет не пустой
        {
            return ""; // и если пустой то возвращае пустой результат
        }
        total_received += bytes_received; // добавляем к переменной-сумме размер текущего пакетика
    }

    return message; // возвращаем сообщение
}

// код который посылает сообщение по сокету
void send_message(int socket, const std::string& message) 
{
    int size = message.size();
    send(socket, (char*)&size, sizeof(int), 0); // Отправляем размер
    send(socket, message.c_str(), size, 0);     // Отправляем само сообщение
}

void Recieve_msg(int connection,Storage& s, ULL &id)
{
    while(true)
    {
        int msg_size = get_message_size(connection);

        if( msg_size <= 0)
        {
            std::cerr << "Сообщение нулевого размера"  << std::endl;
            return;
        }

        std::string msg = get_message(connection,msg_size);

        msg = Decrypt(msg,key,iv);

        if( msg.empty() )
        {
            std::cerr << "Сообщение пустое" << std::endl;
            return;
        }

        std::vector<std::string> parse_com = Parse(msg);
        
        std::string msg_to_send;

        if(parse_com[0] == "exit" || parse_com[0] == "Exit" )
        {
            s.exit_user(id); // Запускаем протокол выхода

            auto it = std::find(clients.begin(), clients.end(), connection); // находим айли номера
            clients.erase(it); // удаляем из списка подключенных клиентов

            std::cout << "Клиент " << connection << " отключился" << std::endl;

            msg_to_send = "Вы покинули сервер";

            msg_to_send = Encrypt(msg_to_send,key,iv);

            send_message(connection,msg_to_send);

            close(connection); //  отключаем клиента

            return;
        }
        else if( parse_com[0]== "-h" || parse_com[0] == "--help" )
        {
            msg_to_send = s.info();

            msg_to_send = Encrypt(msg_to_send,key,iv);

            send_message(connection,msg_to_send);
        }
        else if( parse_com[0] == "-c" || parse_com[0] == "--create")
        {
            if( parse_com.size() == 4 )
            {
                msg_to_send = s.NewData(parse_com[1],parse_com[2],parse_com[3],id);
            }
            else
            {
                msg_to_send = "Ошибка. Неверное количество параметров для метода create\n";
            }

            msg_to_send = Encrypt(msg_to_send,key,iv);
            send_message(connection,msg_to_send);
        }
        else if( parse_com[0] == "--c")
        {
            if( parse_com.size() == 4 )
            {
                msg_to_send = s.NewData(parse_com[1],parse_com[2],parse_com[3],id);
            }
            else
            {
                msg_to_send = "Ошибка. Неверное количество параметров для метода create с автоматической генерацией паролей\n";
            }

            msg_to_send = Encrypt(msg_to_send,key,iv);
            send_message(connection,msg_to_send);
        }
        else if( parse_com[0] == "-f" || parse_com[0] == "--find" )
        {
            if( parse_com.size() == 3 )
            {
                msg_to_send = s.Find(id,parse_com[1],parse_com[2]);
            }
            else if( parse_com.size() == 2 )
            {
                msg_to_send = s.Find(id,parse_com[1]);
            }
            else if( parse_com.size() == 1)
            {
                msg_to_send = s.Find(id);
            }
            else
            {
                msg_to_send = "Ошибка. Неверное количество параметров для метода find\n";
            }

            msg_to_send = Encrypt(msg_to_send,key,iv);
            send_message(connection,msg_to_send);
        }
        else if( parse_com[0] == "-p" || parse_com[0] == "--print")
        {
            if( parse_com.size() == 1 )
            {
                msg_to_send = s.Print(id);
            }
            else if( parse_com.size() == 2)
            {
                msg_to_send = s.Print(id,parse_com[1]);
            }
            else if( parse_com.size() == 3 )
            {
                msg_to_send = s.Print(id,parse_com[1],parse_com[2]);
            }
            else
            {
                msg_to_send = "Ошибка. Неверное количество параметров для метода print\n";
            }

            msg_to_send = Encrypt(msg_to_send,key,iv);
            send_message(connection,msg_to_send);
        }
        else if( parse_com[0] == "-e" || parse_com[0] == "--edit" )
        {
            if( parse_com.size() == 4 )
            {
                msg_to_send = s.Edit(id,parse_com[1],parse_com[2],parse_com[3]);
            }
            else
            {
                msg_to_send = "Ошибка. Неверное количество параметров для метода edit\n";
            }

            msg_to_send = Encrypt(msg_to_send,key,iv);
            send_message(connection,msg_to_send);
        }
        else if( parse_com[0] == "-d" || parse_com[0] == "--delete" )
        {
            if( parse_com.size() == 3 )
            {
                msg_to_send = s.Delete(id,parse_com[1],parse_com[2]);
            }
            else if( parse_com.size() == 1 ) // Т.е удаление всей ячейки юзера.
            {
                msg_to_send = s.Delete(id);
            } 
            else
            {
                msg_to_send = "Ошибка. Неверное количество параметров для метода delete\n";
            }

            msg_to_send = Encrypt(msg_to_send,key,iv);
            send_message(connection,msg_to_send);
        }
        else
        {
            msg_to_send = "Некорректный ввод команды или данных!\n";

            msg_to_send = Encrypt(msg_to_send,key,iv);
            send_message(connection,msg_to_send);
        }
    }
}

int main()
{
    srand(time(0));

    Storage s;

    int server_sock = socket(AF_INET, SOCK_STREAM,0);

    if(server_sock == -1)
    {
        std::cerr << "Ошибка создания сокета." << std::endl;
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1111);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(bind(server_sock,(sockaddr*)&server_addr,sizeof(server_addr)))
    {
        std::cerr << "Ошибка инициализации сервера!" << std::endl;
        return -1;
    }

    if(listen(server_sock, MAX_CLIENTS) < 0)
    {
        std::cerr << "При прослушке произошла ошибка." << std::endl;
        return -1;
    }

    std::cout << "Сервер прослушивается на порту 1111..." << std::endl;

    std::vector<std::thread> pool_threads;
    int newConnection;
    socklen_t client_len = sizeof(sockaddr_in);
    std::string msg_to_client,msg_from_client;
    int msg_size;
    std::vector<std::string> vector_data;

    /*
        Авторизация должна быть в отдельном треде
    */

    while(true)
    {
        newConnection = accept(server_sock,(sockaddr*)&server_addr, &client_len); //682 - 759 процесс авторизации

        if( newConnection <= 0 )
        {
            std::cerr << "Ошибка соединения" << std::endl;
            return -1;
        }

        msg_size = get_message_size(newConnection);
        msg_from_client = get_message(newConnection,msg_size);

        msg_from_client = Decrypt(msg_from_client,key,iv);

        std::cout << msg_from_client << std::endl;

        std::vector<std::string> parse_com = Parse(msg_from_client);

        ULL id = s.autentificate(parse_com[0],parse_com[1]);
        if( id != -1 )
        {
            msg_to_client = "Добро пожаловать в систему.";
            msg_to_client = Encrypt(msg_to_client,key,iv);
            msg_size = msg_to_client.size();

            send(newConnection,(char*)&msg_size,sizeof(int),0);
            send(newConnection,msg_to_client.c_str(),msg_size,0);

            s.enter_user(id);
        }
        else
        {
            msg_to_client = "Ошибка ввода. Хотите зарегестрироваться?(Введите Да или Нет): ";
            msg_to_client = Encrypt(msg_to_client,key,iv);

            msg_size = msg_to_client.size();

            send(newConnection,(char*)&msg_size,sizeof(int),0);
            send(newConnection,msg_to_client.c_str(),msg_size,0);

            msg_size = get_message_size(newConnection);
            msg_from_client = get_message(newConnection,msg_size);

            msg_from_client = Decrypt(msg_from_client,key,iv);

            if( msg_from_client == "Да" )
            {
                id = s.create_user(parse_com[0],parse_com[1]);

                msg_to_client = "Добро пожаловать в систему.";
                msg_size = msg_to_client.size();
                msg_to_client = Encrypt(msg_to_client,key,iv);

                send(newConnection,(char*)&msg_size,sizeof(int),0);
                send(newConnection,msg_to_client.c_str(),msg_size,0);
            }
            else
            {
                msg_to_client = "Ошибка аутентификации. Неверный логин или пароль. Выход";
                msg_size = msg_to_client.size();
                msg_to_client = Encrypt(msg_to_client,key,iv);

                send(newConnection,(char*)&msg_size,sizeof(int),0);
                send(newConnection,msg_to_client.c_str(),msg_size,0);

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                close(newConnection);
            }
        }

        if(1 + pool_threads.size() < MAX_CLIENTS)
        {
            clients.push_back(newConnection);

            std::cout << "Клиент " << newConnection << " присоединился" << std::endl;

            std::thread t(Recieve_msg,newConnection,std::ref(s),std::ref(id));
            pool_threads.push_back(std::move(t));
        }
        else
        {
            std::cerr << "Сервер переполнен. Подсоединение невозможно!" << std::endl;
        }
    }

    for(auto &it: pool_threads)
    {
        if(it.joinable())
        {
            it.join();
        }
    }

    return 0;
}