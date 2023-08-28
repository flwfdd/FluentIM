//
// Created by flwfdd on 2023/8/27.
//

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include "database.h"
#include "global/store.h"

Database::Database() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("fluentchat.db");
    if (!db.open()) {
        qDebug() << "Error: Failed to connect database." << db.lastError();
    } else {
        qDebug() << "Succeed to connect database." << db.lastError();
    }

    QSqlQuery query;

    // 删除所有表
//    QString dropTableQuery = "DROP TABLE IF EXISTS `user`";
//    if (!query.exec(dropTableQuery)) {
//        qDebug() << "Failed to drop table:" << query.lastError().text();
//    }
//    dropTableQuery = "DROP TABLE IF EXISTS `group`";
//    if (!query.exec(dropTableQuery)) {
//        qDebug() << "Failed to drop table:" << query.lastError().text();
//    }
//    dropTableQuery = "DROP TABLE IF EXISTS `message`";
//    if (!query.exec(dropTableQuery)) {
//        qDebug() << "Failed to drop table:" << query.lastError().text();
//    }

    // 创建用户表
    QString createTableQuery = "CREATE TABLE IF NOT EXISTS `user` ("
                               "`id` INTEGER NOT NULL PRIMARY KEY, "
                               "`username` VARCHAR(255) NOT NULL, "
                               "`nickname` VARCHAR(255) NOT NULL, "
                               "`color` VARCHAR(255) NOT NULL, "
                               "`avatar` VARCHAR(255) NOT NULL)";
    if (!query.exec(createTableQuery)) {
        qDebug() << "Failed to create table:" << query.lastError().text();
    }

    // 创建群组表
    createTableQuery = "CREATE TABLE IF NOT EXISTS `group` ("
                       "`id` INTEGER NOT NULL PRIMARY KEY, "
                       "`type` VARCHAR(255) NOT NULL, "
                       "`name` VARCHAR(255) NOT NULL, "
                       "`avatar` VARCHAR(255) NOT NULL, "
                       "`color` VARCHAR(255) NOT NULL, "
                       "`owner` INTEGER NOT NULL, "
                       "`last` INTEGER, " // 最后一条消息的id
                       "`read` INTEGER DEFAULT 0)";
    if (!query.exec(createTableQuery)) {
        qDebug() << "Failed to create table:" << query.lastError().text();
    }

    // 创建消息表
    createTableQuery = "CREATE TABLE IF NOT EXISTS `message` ("
                       "`id` INTEGER NOT NULL PRIMARY KEY, "
                       "`type` VARCHAR(255) NOT NULL, "
                       "`content` TEXT NOT NULL, "
                       "`time` INTEGER NOT NULL, "
                       "`uid` INTEGER NOT NULL, "
                       "`gid` INTEGER NOT NULL, "
                       "`mid` INTEGER NOT NULL, "
                       "`recall` BOOLEAN NOT NULL)";
    if (!query.exec(createTableQuery)) {
        qDebug() << "Failed to create table:" << query.lastError().text();
    }
}

Database *Database::instance() {
    static Database instance;
    return &instance;
}

void Database::saveUsers(QList<UserModel *> users) {
    if (users.isEmpty())return;
    QSqlQuery query;
    QString insertQuery = "INSERT OR REPLACE INTO `user` (`id`, `username`, `nickname`, `color`, `avatar`) VALUES ";
    for (auto user: users) {
        insertQuery +=
                "(" + QString::number(user->id()) + ", '" + user->username() + "', '" + user->nickname() + "', '" +
                user->color() + "', '" + user->avatar() + "'),";
    }
    insertQuery = insertQuery.left(insertQuery.length() - 1);
    if (!query.exec(insertQuery)) {
        qDebug() << "Failed to insert users:" << query.lastError().text();
    }
}

QList<UserModel *> Database::loadUsers(QList<UserModel *> users) {
    if (users.empty())return {};
    QMap<int, UserModel *> map;
    for (auto user: users) {
        map.insert(user->id(), user);
    }
    QList<UserModel *> loadedUsers;
    QSqlQuery query;
    QString selectQuery = "SELECT * FROM `user` WHERE `id` IN (";
    for (auto id: map.keys()) {
        selectQuery += QString::number(id) + ",";
    }
    selectQuery = selectQuery.left(selectQuery.length() - 1) + ")";
    if (!query.exec(selectQuery)) {
        qDebug() << "Failed to load users:" << query.lastError().text();
    } else {
        while (query.next()) {
            auto user = map.value(query.value(0).toInt());
            user->setUsername(query.value(1).toString());
            user->setNickname(query.value(2).toString());
            user->setColor(query.value(3).toString());
            user->setAvatar(query.value(4).toString());
            loadedUsers.append(user);
        }
    }
    return loadedUsers;
}

void Database::saveGroups(QList<GroupModel *> groups) {
    if (groups.isEmpty())return;
    QSqlQuery query;
    QString insertQuery = "INSERT OR REPLACE INTO `group` (`id`, `type`, `name`, `avatar`, `color`, `owner`, `last`) VALUES ";
    for (auto group: groups) {
        insertQuery +=
                "(" + QString::number(group->id()) + ", '" + group->type() + "', '" + group->name() + "', '" +
                group->avatar() + "', '" + group->color() + "', " + QString::number(group->owner()->id()) + ", " +
                (group->last() ? QString::number(group->last()->id()) : "NULL") + "),";
    }
    insertQuery = insertQuery.left(insertQuery.length() - 1);
    if (!query.exec(insertQuery)) {
        qDebug() << "Failed to insert groups:" << query.lastError().text();
    }
}

QList<GroupModel *> Database::getGroups() {
    QList<GroupModel *> groups;
    QSqlQuery query;
    QString selectQuery = "SELECT * FROM `group`";
    if (!query.exec(selectQuery)) {
        qDebug() << "Failed to load groups:" << query.lastError().text();
    } else {
        while (query.next()) {
            auto group = new GroupModel();
            group->setId(query.value(0).toInt());
            group->setType(query.value(1).toString());
            group->setName(query.value(2).toString());
            group->setAvatar(query.value(3).toString());
            group->setColor(query.value(4).toString());
            group->setOwner(Control::instance()->getUsers(QList<int>() << query.value(5).toInt())[0]);
            auto message_id = query.value(6).toInt();
            group->setLast(getMessage(message_id));
            group->setRead(query.value(7).toInt());
            groups.append(group);
        }
    }
    return groups;
}

void Database::saveMessages(QList<MessageModel *> messages) {
    if (messages.isEmpty())return;
    QSqlQuery query;
    QString insertQuery = "INSERT OR REPLACE INTO `message` (`id`, `type`, `content`, `time`, `uid`, `gid`, `mid`, `recall`) VALUES ";
    for (auto message: messages) {
        insertQuery +=
                "(" + QString::number(message->id()) + ", '" + message->type() + "', '" + message->content() + "', " +
                QString::number(message->time()) + ", " + QString::number(message->user()->id()) + ", " +
                QString::number(message->gid()) + ", " + QString::number(message->mid()) + ", " +
                QString::number(message->recall()) + "),";
    }
    insertQuery = insertQuery.left(insertQuery.length() - 1);
    if (!query.exec(insertQuery)) {
        qDebug() << "Failed to insert messages:" << query.lastError().text();
    }
}

void loadMessageFromQuery(QSqlQuery &query, MessageModel *message) {
    message->setId(query.value(0).toInt());
    message->setType(query.value(1).toString());
    message->setContent(query.value(2).toString());
    message->setTime(query.value(3).toInt());
    message->setUser(Control::instance()->getUsers(QList<int>() << query.value(4).toInt())[0]);
    message->setGid(query.value(5).toInt());
    message->setMid(query.value(6).toInt());
    message->setRecall(query.value(7).toBool());
}

MessageModel *Database::getMessage(int id) {
    QSqlQuery query;
    QString selectQuery = "SELECT * FROM `message` WHERE `id` = " + QString::number(id);
    if (!query.exec(selectQuery)) {
        qDebug() << "Failed to load message:" << query.lastError().text();
    } else {
        if (query.next()) {
            auto message = new MessageModel();
            loadMessageFromQuery(query, message);
            return message;
        }
    }
    return nullptr;
}

QList<MessageModel *> Database::loadMessages(int gid, int start, int end) {
    QList<MessageModel *> messages;
    QSqlQuery query;
    QString selectQuery = "SELECT * FROM `message` WHERE `gid` = " + QString::number(gid) + " AND `mid` >= " +
                          QString::number(start) + " AND `mid` <= " + QString::number(end);
    if (!query.exec(selectQuery)) {
        qDebug() << "Failed to load messages:" << query.lastError().text();
    } else {
        while (query.next()) {
            auto message = new MessageModel();
            loadMessageFromQuery(query, message);
            messages.append(message);
        }
    }
    return messages;
}
