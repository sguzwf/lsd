	Json::Value object(Json::objectValue);
	object["uuid"] = "bb8d8bc6-864b-4ffc-ae28-b3edc4c35ad7";
	object["protocol_version"] = "1.0";

	Json::Value acc_data(Json::arrayValue);
	acc_data.append(Json::Value(1));
	acc_data.append(Json::Value(2));
	acc_data.append(Json::Value(3));

	Json::Value message(Json::objectValue);
	message["user_account_id"] = "87657687";
	message["account_action"] = "create";
	message["account_data"] = acc_data;

	object["message"] = message;

	std::string str = object.toStyledString();
	std::cout << "created json str:\n" << str << std::endl;

	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(str, root);

	std::cout << "parsed json str:" << std::endl;
	if (!parsingSuccessful) {
		std::cout << "epic fail!" << std::endl;
	}

	std::cout << root;
	std::string uuid = root.get("uuid", "UTF-8" ).asString();
	std::string proto_ver = root.get("protocol_version", "UTF-8" ).asString();
	std::cout << "uuid: " << uuid << std::endl;
	std::cout << "protocol_version: " << proto_ver << std::endl;

	const Json::Value mgs_obj = root["message"];

	std::string user_account_id = mgs_obj.get("user_account_id", "UTF-8").asString();
	std::string account_action = mgs_obj.get("account_action", "UTF-8").asString();
	std::cout << "user_account_id: " << user_account_id << std::endl;
	std::cout << "account_action: " << account_action << std::endl;

	const Json::Value data = mgs_obj["account_data"];
	for (size_t index = 0; index < data.size(); ++index) {
		std::cout << index << ": " << data[index].asInt() << std::endl;
	}