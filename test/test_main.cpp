#include <gtest/gtest.h>
#include "parser.hpp"
#include <iostream>
#include <string>

namespace parser {
  namespace test {

    class TestListener : public IListener { 
    public:
        std::vector<std::vector<std::string>> received_blocks;
        
        void update(const std::vector<std::string>& block, [[maybe_unused]] time_t time) override {
            received_blocks.push_back(block);
        }
    };

    class PackHandlerTest : public ::testing::Test {
    protected:
        size_t default_pack_size = 3;
        std::unique_ptr<PackHandler> handler;
        std::shared_ptr<TestListener> spy_listener;
    
        void SetUp() override {
            handler = std::make_unique<PackHandler>(default_pack_size);
            spy_listener = std::make_shared<TestListener>();
            handler->registerListener(spy_listener); 
        }
    
       
    };


    TEST_F(PackHandlerTest, FlushesStaticPackWhenSizeReached) {
        handler->add_cmd_to_pack("cmd1");
        handler->add_cmd_to_pack("cmd2");
      
        EXPECT_TRUE(spy_listener->received_blocks.empty());
    
        handler->add_cmd_to_pack("cmd3");
    
        ASSERT_EQ(spy_listener->received_blocks.size(), 1);
        EXPECT_EQ(spy_listener->received_blocks[0], std::vector<std::string>({"cmd1", "cmd2", "cmd3"}));
    }
    
    TEST_F(PackHandlerTest, HandlesDynamicPackWithBrackets) {
        handler->add_cmd_to_pack("cmd1"); 
        handler->add_cmd_to_pack("{");  
        ASSERT_EQ(spy_listener->received_blocks.size(), 1);
        EXPECT_EQ(spy_listener->received_blocks[0], std::vector<std::string>({"cmd1"}));
    
        handler->add_cmd_to_pack("cmd2");
        handler->add_cmd_to_pack("cmd3");
        handler->add_cmd_to_pack("cmd4");
    
        handler->add_cmd_to_pack("}");    
        ASSERT_EQ(spy_listener->received_blocks.size(), 2);
        EXPECT_EQ(spy_listener->received_blocks[1], std::vector<std::string>({"cmd2", "cmd3", "cmd4"}));
    }
    
    TEST_F(PackHandlerTest, FlushesOnEOFOnlyInStaticState) {
        handler->add_cmd_to_pack("cmd1");
        handler->flush_eof(); 
    
        ASSERT_EQ(spy_listener->received_blocks.size(), 1);
        EXPECT_EQ(spy_listener->received_blocks[0], std::vector<std::string>({"cmd1"}));
    }

  }
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
