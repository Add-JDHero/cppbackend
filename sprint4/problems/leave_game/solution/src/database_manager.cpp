#include "database_manager.h"

namespace db {
    db::ConnectionPool CreateConnectionPool(unsigned num_threads, const std::string& db_url) {
        return db::ConnectionPool{
            num_threads, [db_url] {
                return std::make_shared<pqxx::connection>(db_url);
            }
        };
    }

    void InitDatabase(db::ConnectionPool& conn_pool) {
        auto connection = conn_pool.GetConnection();
        
        pqxx::work work{ *connection };

        work.exec(R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id UUID DEFAULT gen_random_uuid() PRIMARY KEY, 
                name VARCHAR(100) NOT NULL, 
                score INTEGER, 
                play_time_ms INTEGER
            );
        )");

        work.exec(R"(
            CREATE INDEX IF NOT EXISTS retired_players_idx 
            ON retired_players USING btree (score DESC, play_time_ms, name);
        )");

        work.commit();
    }

    void RegisterPrepQueries(ConnectionPool::ConnectionWrapper conn_wrap) {
        conn_wrap->prepare(
            "insert_retire", 
            "INSERT INTO retired_players (name, score, play_time_ms) VALUES ($1, $2, $3)"
        );
    }

    template <typename ConnectionFactory>
    ConnectionPool::ConnectionPool(size_t capacity, 
                                   ConnectionFactory&& connection_factory) {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(connection_factory());
        }
        
        RegisterPrepQueries(GetConnection());
    }

    ConnectionPool::ConnectionWrapper ConnectionPool::GetConnection() {
        std::unique_lock lock{mutex_};
        // Блокируем текущий поток и ждём, пока cond_var_ не получит 
        // уведомление и не освободится хотя бы одно соединение
        cond_var_.wait(lock, [this] {
            return used_connections_ < pool_.size();
        });
        // После выхода из цикла ожидания мьютекс остаётся захваченным

        return {std::move(pool_[used_connections_++]), *this};
    }

    void ConnectionPool::ReturnConnection(ConnectionPtr&& conn) {
        // Возвращаем соединение обратно в пул
        {
            std::lock_guard lock{mutex_};
            assert(used_connections_ != 0);
            pool_[--used_connections_] = std::move(conn);
        }
        // Уведомляем один из ожидающих потоков об изменении состояния пула
        cond_var_.notify_one();
    }
}