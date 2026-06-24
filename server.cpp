#include "json.hpp"
#include "QueryProcessor/QueryProcessor.h"
#define _WIN32_WINNT 0x0A00
#include "httplib.h"

using json = nlohmann::json;

int main() {
    QueryProcessor qp("./catalog", "./data");
    httplib::Server svr;

    // CORS
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Post("/query", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string sql = body["query"];

            auto result = qp.execute(sql);

            json response;
            response["success"] = result.success;
            response["message"] = result.message;
            response["columns"] = result.columns;

            json rows = json::array();
            for (const auto& row : result.rows) {
                json r = json::array();
                for (int i = 0; i < row.fieldCount(); i++)
                    r.push_back(row.getField(i));
                rows.push_back(r);
            }
            response["rows"] = rows;

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            json err;
            err["success"] = false;
            err["message"] = std::string("Server error: ") + e.what();
            err["columns"] = json::array();
            err["rows"] = json::array();
            res.set_content(err.dump(), "application/json");
            res.status = 500;
        }
    });

    std::cout << "TinySQLDb server corriendo en http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}