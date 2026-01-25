#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <variant>

#include "assets.h"
#include "resultError.h"
#include "btContext.h"

namespace df {
	class CommandRegistry;
	/* Assumptions:
	 * - The players are the only actors needing an AI
	 * - The game is round-based
	 */


	class BTNode {
	public:
		virtual ~BTNode() = default;
		virtual void init(Agent) {};
		virtual BTState process(BTContext&) = 0;
		virtual nlohmann::json serialize() const = 0;
		static std::shared_ptr<BTNode> deserialize(const nlohmann::json&, const CommandRegistry&);
		virtual bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) = 0;
		static Result<std::shared_ptr<BTNode>, ResultError> deserialize(const CommandRegistry&, assets::JsonFile asset = assets::JsonFile::WORLD_GENERATION_CONFIGURATION);
	};


	class BTSequence final : public BTNode {
	public:
		BTSequence() = default;
		explicit BTSequence(const std::vector<std::shared_ptr<BTNode>> &children) : children(children) {}
		void init(Agent) override;
		BTState process(BTContext&) override;
		nlohmann::json serialize() const override;
		bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) override;
	private:
		std::vector<std::shared_ptr<BTNode>> children;
		std::map<Agent, unsigned> currentChildIndex;
	};


	class BTSelector final : public BTNode {
	public:
		BTSelector() = default;
		explicit BTSelector(const std::vector<std::shared_ptr<BTNode>> &children) : children(children) {}
		void init(Agent) override;
		BTState process(BTContext&) override;
		nlohmann::json serialize() const override;
		bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) override;
	private:
		std::vector<std::shared_ptr<BTNode>> children;
		std::map<Agent, unsigned> currentChildIndex;
	};


	class BTInverter final : public BTNode {
	public:
		BTInverter() = default;
		explicit BTInverter(const std::shared_ptr<BTNode> &child) : child(child) {}
		void init(Agent) override;
		BTState process(BTContext&) override;
		nlohmann::json serialize() const override;
		bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) override;
	private:
		std::shared_ptr<BTNode> child;
	};


	class BTSucceeder final : public BTNode {
	public:
		BTSucceeder() = default;
		explicit BTSucceeder(const std::shared_ptr<BTNode> &child) : child(child) {}
		void init(Agent) override;
		BTState process(BTContext&) override;
		nlohmann::json serialize() const override;
		bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) override;
	private:
		std::shared_ptr<BTNode> child;
	};


	class BTUntilFailureRepeater final : public BTNode {
	public:
		BTUntilFailureRepeater() = default;
		explicit BTUntilFailureRepeater(const std::shared_ptr<BTNode> &child) : child(child) {}
		void init(Agent) override;
		BTState process(BTContext&) override;
		nlohmann::json serialize() const override;
		bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) override;
	private:
		std::shared_ptr<BTNode> child;
	};


	class BTRepeater final : public BTNode {
	public:
		BTRepeater() = default;
		explicit BTRepeater(const std::shared_ptr<BTNode> &child, const unsigned times = 1) : child(child), times(times) {}
		void init(Agent) override;
		BTState process(BTContext&) override;
		nlohmann::json serialize() const override;
		bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) override;
	private:
		std::shared_ptr<BTNode> child;
		unsigned times = 1;
		std::map<Agent, unsigned> counter;
	};


	class BTFunction final : public BTNode {
	public:
		//using JsonType = std::variant<std::string, double, bool>;
		//using Args = std::unordered_map<std::string, JsonType>;
		//using Function = std::function<BTState(Agent, Args)>;

		BTFunction() = default;
		explicit BTFunction(std::string name, const CommandRegistry& commandRegistry);
		void init(Agent) override;
		BTState process(BTContext&) override;
		nlohmann::json serialize() const override;
		bool deserializeInplace(const nlohmann::json&, const CommandRegistry&) override;
	private:
		std::string name = "success";
		BTF::Args args{};
		BTF::Command fn = [](BTContext&, const BTF::Args&){ return BTState::Success; };
	};
}
