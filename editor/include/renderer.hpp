#include "common.hpp"
#include "document.hpp"

class Renderer : public Observer {
    public:
        explicit Renderer(Document& document) : document_(document) {
            document_.add_observer(*this);
        }
        void update() override {
            render();
        }
    private:
        Document& document_;
        
        void render(){
            std::cout << "rendering view" << std::endl;
            for (const auto& pair : document_.get_all_shapes()) {
                pair.second->draw();
            }
        }
};
