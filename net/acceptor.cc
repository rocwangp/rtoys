#include "acceptor.h"
#include "channel.h"
#include "eventloop.h"

#include "../ip/sockets.h"
#include "../util/io.h"
#include "../util/logging.h"

namespace rtoys
{
    namespace net
    {
        Acceptor::Acceptor(EventLoop* loop, const std::string& ip, unsigned short port)
            : loop_(loop),
              channel_(std::make_unique<Channel>(loop)),
              idleFd_(util::io::open("dev/null"))
        {
            channel_->onRead(
                            [this]
                            {
                                int fd = rtoys::ip::tcp::socket::accept(channel_->fd());
                                if(fd == -1)
                                {
                                    if(errno == EMFILE)
                                    {
                                        util::io::close(idleFd_);
                                        fd = rtoys::ip::tcp::socket::accept(channel_->fd());
                                        rtoys::ip::tcp::socket::close(fd);
                                        idleFd_ = util::io::open("dev/null");
                                        fd = -1;
                                        log_error("fd use out...");
                                    }
                                    
                                    return;
                                }
                                if(acceptCallBack_)
                                    this->acceptCallBack_(fd);
                            }
                        );
            rtoys::ip::tcp::socket::reuse_port(channel_->fd());
            rtoys::ip::tcp::socket::reuse_address(channel_->fd());
            auto address = std::make_shared<rtoys::ip::tcp::address::v4>(ip, port);
            if(!rtoys::ip::tcp::socket::bind(channel_->fd(), address))
            {
                log_error(ip, " ", port);
                throw std::runtime_error("bind error ");
            }
            if(!rtoys::ip::tcp::socket::listen(channel_->fd()))
            {
                log_error(ip, " ", port);
                throw std::runtime_error("listen error");
            }
        }

        Acceptor::~Acceptor()
        {
            channel_->disableAll();
            util::io::close(idleFd_);    
        }

        void Acceptor::start()
        {
            /* log_trace; */
            channel_->enableRead();
        }
    }
}
