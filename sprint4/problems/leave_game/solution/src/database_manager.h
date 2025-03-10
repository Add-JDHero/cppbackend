#pragma once
#include <memory>

#include <cassert>
#include <condition_variable>
#include <mutex>

#include <pqxx/pqxx>

namespace db {
    // using pqxx::operator"" _zv;

    class ConnectionPool;
    class ConnectionWrapper;

    ConnectionPool CreateConnectionPool(unsigned num_threads, const std::string& db_url);

    void InitDatabase(db::ConnectionPool& conn_pool);

    void RegisterPrepQueries(ConnectionWrapper connection);

    class ConnectionPool {
        using PoolType = ConnectionPool;
        using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    public:
        class ConnectionWrapper {
        public:
            ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
                : conn_{std::move(conn)}
                , pool_{&pool} {
            }

            ConnectionWrapper(const ConnectionWrapper&) = delete;
            ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

            ConnectionWrapper(ConnectionWrapper&&) = default;
            ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

            pqxx::connection& operator*() const& noexcept {
                return *conn_;
            }
            pqxx::connection& operator*() const&& = delete;

            pqxx::connection* operator->() const& noexcept {
                return conn_.get();
            }

            ~ConnectionWrapper() {
                if (conn_) {
                    pool_->ReturnConnection(std::move(conn_));
                }
            }

        private:
            std::shared_ptr<pqxx::connection> conn_;
            PoolType* pool_;
        };

        // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
        template <typename ConnectionFactory>
        ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory);

        ConnectionWrapper GetConnection() ;

    private:
        void ReturnConnection(ConnectionPtr&& conn) ;

        std::mutex mutex_;
        std::condition_variable cond_var_;
        std::vector<ConnectionPtr> pool_;
        size_t used_connections_ = 0;
    };
} //namespace db