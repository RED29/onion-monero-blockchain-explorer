
#include "src/CmdLineOptions.h"
#include "src/MicroCore.h"
#include "src/page.h"

#include "ext/crow/crow.h"
#include "ext/member_checker.h"

#include <fstream>

using boost::filesystem::path;

using namespace std;

// needed for log system of momero
namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}

int main(int ac, const char* av[]) {

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return EXIT_SUCCESS;
    }

    auto port_opt           = opts.get_option<string>("port");
    auto bc_path_opt        = opts.get_option<string>("bc-path");
    auto custom_db_path_opt = opts.get_option<string>("custom-db-path");
    auto deamon_url_opt     = opts.get_option<string>("deamon-url");

    //cast port number in string to uint16
    uint16_t app_port = boost::lexical_cast<uint16_t>(*port_opt);

    // get blockchain path
    path blockchain_path;

    if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path))
    {
        cerr << "Error getting blockchain path." << endl;
        return EXIT_FAILURE;
    }

     // enable basic monero log output
    xmreg::enable_monero_log();

    // create instance of our MicroCore
    // and make pointer to the Blockchain
    xmreg::MicroCore mcore;
    cryptonote::Blockchain* core_storage;

    // initialize mcore and core_storage
    if (!xmreg::init_blockchain(blockchain_path.string(),
                               mcore, core_storage))
    {
        cerr << "Error accessing blockchain." << endl;
        return EXIT_FAILURE;
    }

    // create instance of page class which
    // contains logic for the website
    xmreg::page xmrblocks(&mcore, core_storage, *deamon_url_opt);

    // crow instance
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([&]() {
        return xmrblocks.index2();
    });

    CROW_ROUTE(app, "/page/<uint>")
    ([&](size_t page_no) {
        return xmrblocks.index2(page_no);
    });

    CROW_ROUTE(app, "/block/<uint>")
    ([&](size_t block_height) {
        return xmrblocks.show_block(block_height);
    });

    CROW_ROUTE(app, "/block/<string>")
    ([&](string block_hash) {
        return xmrblocks.show_block(block_hash);
    });

    CROW_ROUTE(app, "/tx/<string>")
    ([&](string tx_hash) {
        return xmrblocks.show_tx(tx_hash);
    });

    CROW_ROUTE(app, "/tx/<string>/<uint>")
    ([&](string tx_hash, uint with_ring_signatures) {
        return xmrblocks.show_tx(tx_hash, with_ring_signatures);
    });


    CROW_ROUTE(app, "/search").methods("GET"_method)
    ([&](const crow::request& req) {
        return xmrblocks.search(string(req.url_params.get("value")));
    });

    CROW_ROUTE(app, "/autorefresh")
    ([&]() {
        uint64_t page_no {0};
        bool refresh_page {true};
        return xmrblocks.index2(page_no, refresh_page);
    });

    // run the crow http server
    app.port(app_port).multithreaded().run();

    return EXIT_SUCCESS;
}
