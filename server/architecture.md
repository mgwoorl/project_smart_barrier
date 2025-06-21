users {
    id int primary key !nullable,
    chat_id int !nullable,
    isAdmin bool !nullable,
    deleted_at datetime
}

devices_data {
    id int primary key !nullable,
    distance_exit int nullable,  // Сделать запрос на обновление данных
    distance_entrance int nullable, // Сделать запрос на обновление данных
    free_places int nullable, // Сделать запрос на обновление данных
    co2 int nullable // Сделать запрос на обновление данных
}

system {
    id int primary key !nullable,
    isEntranceBlock bool !nullable  // Сделать запрос на обновление статуса
    isWannaOpen bool !nullable
}


Telegram commands:
Admin:
1) register user
2) remove user
3) get statistics
4) init system

User:
1) open


General:
1) get co2
2) get count places


mysql -u root -p

user