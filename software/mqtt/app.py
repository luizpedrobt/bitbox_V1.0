import streamlit as st

st.title("Datalogger Viewer")

if "logs" not in st.session_state:
    st.session_state.logs = []

# função que adiciona linha
def add_log(line):
    st.session_state.logs.append(line)

# exemplo (simulando dados)
if st.button("Adicionar teste"):
    add_log("Exemplo de dado chegando")

# mostrar logs
for log in reversed(st.session_state.logs[-50:]):
    st.text(log)