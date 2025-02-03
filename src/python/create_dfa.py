from fastapi import FastAPI
from pyformlang.regular_expression import Regex
from pyformlang.finite_automaton import State
from typing import Dict, Any

app = FastAPI()

@app.get("/create_dfa")
def create_dfa(regex: str) -> Dict[str, Any]:
    """
    Creates a DFA based on a RegEx.
    Transforms pyformlang states to strings.
    """
    regex = Regex(regex)

    _nfa = regex.to_epsilon_nfa().remove_epsilon_transitions()
    dfa = _nfa.to_deterministic().minimize()
    states = [] 
    
    for state in dfa.states:
        states.append(state.value)

    dfa_dict = dfa.to_dict()

    START_STATE = dfa.start_state.value
    FINAL_STATES = [final_state.value for final_state in dfa.final_states]
    
    converted_dfa_dict = {}
    for state, transitions in dfa_dict.items():
        state_str = state.value if isinstance(state, State) else state
        converted_transitions = {}
        for symbol, next_state in transitions.items():
            converted_symbol = symbol.value
            next_state_str = next_state.value if isinstance(next_state, State) else next_state
            converted_transitions[converted_symbol] = next_state_str
        converted_dfa_dict[state_str] = converted_transitions

    return {"dfa_dict": converted_dfa_dict, "start_state": START_STATE, "states": states, "final_states": FINAL_STATES}