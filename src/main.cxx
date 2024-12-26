#include <iostream>
#include <cstdlib> // for std::getenv
#include <boost/asio.hpp>
#include "database.hxx"
#include "server.hxx"
#include "logger.hxx"

int main()
{
    auto logger = Logger::getLogger();

    std::string dbHost     = std::getenv("DB_HOST")     ? std::getenv("DB_HOST")     : "localhost";
    std::string dbPort     = std::getenv("DB_PORT")     ? std::getenv("DB_PORT")     : "5432";
    std::string dbName     = std::getenv("DB_NAME")     ? std::getenv("DB_NAME")     : "testdb";
    std::string dbUser     = std::getenv("DB_USER")     ? std::getenv("DB_USER")     : "testuser";
    std::string dbPassword = std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD") : "testpassword";

    std::string connString = 
        "host=" + dbHost + 
        " port=" + dbPort +
        " dbname=" + dbName +
        " user=" + dbUser +
        " password=" + dbPassword;

    auto db = std::make_shared<Database>(connString);
    if (!db->connect())
    {
        logger->error("Failed to connect to the database. Exiting...");
        return 1;
    }

    try
    {
        boost::asio::io_context ioc;

        // Let's choose a port from ENV too, default is 8080
        unsigned short port = 8080;
        if(const char* envPort = std::getenv("SERVER_PORT")) {
            port = static_cast<unsigned short>(std::stoi(envPort));
        }

        Server server(ioc, port, db);
        server.start();

        ioc.run();
    }
    catch(std::exception& e)
    {
        logger->error("Exception in main: {}", e.what());
    }

    return 0;
}
