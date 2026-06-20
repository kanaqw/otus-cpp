#pragma once
#include "common.hpp"
#include "document.hpp"

class Controller {
    public:
        explicit Controller() : document_(std::make_unique<Document>()) {}
        void createDocument() {
            document_ = std::make_unique<Document>();
            std::cout << "New document created." << std::endl;
        }

        void ImportDocument() {
            std::cout << "Importing document" << std::endl;
        }

        void ExportDocument() {
            std::cout << "Exporting document" << std::endl; 
        }

        void createCircleShape() {
            std::cout << "Creating shape" << std::endl;
            document_->add_shape(std::make_unique<Circle>());
        }

        void removeShape(int id) {
            std::cout << "Removing shape" << std::endl;
            document_->remove_shape(id);
        }

        Document& getDocument() {
            return *document_;
        }


    private:
        std::unique_ptr<Document> document_;

};
