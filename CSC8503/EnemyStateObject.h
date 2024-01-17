#pragma once
#include "GameObject.h"

namespace NCL {
    namespace CSC8503 {
        class StateMachine;
        class EnemyStateObject : public GameObject {
        public:
            EnemyStateObject(GameObject* playerObject, int objectTag);
            ~EnemyStateObject();

            virtual void Update(float dt);

        protected:
            void Patrolling(float dt);
            void AttackPlayer(float dt);

            StateMachine* stateMachine;
            GameObject* player;

            Vector3 randomPos;
        };
    }
}

