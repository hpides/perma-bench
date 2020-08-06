#pragma once

namespace nvmbm {

class IoOperation {
public:
    virtual void run();
};

class Pause: public IoOperation {
private:
    const unsigned int length;
};

class Read: public IoOperation {
private:
    const unsigned int number_ops;
    const unsigned int access_size;
    const bool random;
};

class Write: public IoOperation {
private:
    const unsigned int number_ops;
    const unsigned int access_size;
    const bool random;
};

}  // namespace nvmbm
