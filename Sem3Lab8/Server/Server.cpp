#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <list>
#include <string>
using namespace boost::asio;
using namespace boost::posix_time;
io_service service;
static unsigned int index = { 0 };

#define MEM_FN(x)       boost::bind(&self_type::x, shared_from_this())
#define MEM_FN1(x,y)    boost::bind(&self_type::x, shared_from_this(),y)
#define MEM_FN2(x,y,z)  boost::bind(&self_type::x, shared_from_this(),y,z)

struct UserList {
	std::list<std::string> _name_surname;
	UserList() {}
	void addToList(std::string user) {
		
		std::string temp;
		temp.append(std::to_string(index));
		temp.append(". ");
		temp.append(user);
		
		_name_surname.push_back(temp);
	}
	std::string getList() {
		std::string connectedUsers = "";
		if (_name_surname.size() != 0) {
			for (auto i = _name_surname.begin(); i != _name_surname.end(); ++i) {
				connectedUsers.append(*i);
				connectedUsers.append("\n");
			}
		}
		connectedUsers.append("~");
		return connectedUsers;
	}
};
UserList user_list_;

class talk_to_client : public boost::enable_shared_from_this<talk_to_client>, boost::noncopyable
{
	typedef talk_to_client self_type;
	talk_to_client() : sock_(service), started_(false) {}
public:
	typedef boost::system::error_code error_code;
	typedef boost::shared_ptr<talk_to_client> ptr;

	void start()
	{
		started_ = true;
		do_read();
	}
	static ptr new_()
	{
		ptr new_(new talk_to_client);
		return new_;
	}
	void stop()
	{
		if (!started_) return;
		started_ = false;
		sock_.close();
	}
	ip::tcp::socket & sock() { return sock_; }
private:
	void on_read(const error_code & err, size_t bytes)
	{
		if (!err)
		{
			index++;
			std::string msg(read_buffer_, bytes);
			msg.pop_back();
			std::cout << std::endl << "Connected user = " << msg << std::endl;
			user_list_.addToList(msg);
			if (msg == "admin") {
				msg = user_list_.getList();
			}
			else {
				msg = "Hello, " + msg;
			}
			if (msg.size() > max_msg) {
				msg = "msg size limit";
			}
			do_write(msg + "~");
		}
		stop();
	}

	void on_write(const error_code & err, size_t bytes)
	{
		do_read();
	}
	void do_read()
	{
		async_read(sock_, buffer(read_buffer_, max_msg),
			MEM_FN2(read_complete, _1, _2), MEM_FN2(on_read, _1, _2));
	}
	void do_write(const std::string & msg)
	{
		std::copy(msg.begin(), msg.end(), write_buffer_);
		sock_.async_write_some(buffer(write_buffer_, msg.size()),
			MEM_FN2(on_write, _1, _2));
	}
	size_t read_complete(const boost::system::error_code & err, size_t bytes)
	{
		
		if (err) return 0;
		bool found = std::find(read_buffer_, read_buffer_ + bytes, '~') < read_buffer_ + bytes;
		return found ? 0 : 1;
	}
private:
	ip::tcp::socket sock_;
	enum { max_msg = 1024 };
	char read_buffer_[max_msg];
	char write_buffer_[max_msg];
	bool started_;
	
};

ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));

void handle_accept(talk_to_client::ptr client, const boost::system::error_code & err)
{
	client->start();
	talk_to_client::ptr new_client = talk_to_client::new_();
	acceptor.async_accept(new_client->sock(), boost::bind(handle_accept, new_client, _1));
}


int main()
{
	std::cout << "Server is running....\n";
	talk_to_client::ptr client = talk_to_client::new_();
	acceptor.async_accept(client->sock(), boost::bind(handle_accept, client, _1));
	service.run();

	return 0;
}