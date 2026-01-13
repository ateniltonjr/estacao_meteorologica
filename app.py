import tkinter as tk
from tkinter import ttk
import requests
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# ================= CONFIGURAÇÃO =================
PICO_IP = "http://10.20.20.3"
UPDATE_INTERVAL = 1000
# ================================================

historico = []
pagina_atual = None
fullscreen = False

# ================= FUNÇÕES HTTP =================
def obter_estado():
    global historico, pagina_atual
    try:
        r = requests.get(f"{PICO_IP}/estado", timeout=2)
        data = r.json()

        # ---- Página sensores ----
        nome_label.config(text=data["nome"])
        valor_label.config(
            text=f'{data["valor"]:.2f} {data.get("unidade","")}'
        )

        historico = data["historico"]
        pagina_atual = data["pagina"]

        lim_min_var.set(data["lim_min"])
        lim_max_var.set(data["lim_max"])
        offset_var.set(data["offset"])

        atualizar_grafico()

        # ---- Página potenciômetros ----
        pot1_var.set(f'{data["pot1"]:.1f} %')
        pot2_var.set(f'{data["pot2"]:.1f} %')

        pot1_bar["value"] = data["pot1"]
        pot2_bar["value"] = data["pot2"]

    except:
        valor_label.config(text="Sem conexão")

    root.after(UPDATE_INTERVAL, obter_estado)


def enviar_config():
    payload = {
        "pagina": pagina_atual,
        "lim_min": float(lim_min_var.get()),
        "lim_max": float(lim_max_var.get()),
        "offset": float(offset_var.get())
    }
    try:
        requests.post(f"{PICO_IP}/config", json=payload, timeout=2)
    except:
        pass


def mudar_pagina():
    try:
        requests.post(f"{PICO_IP}/pagina", timeout=2)
    except:
        pass

# ================= GRÁFICO =================
def atualizar_grafico():
    ax.clear()

    if len(historico) < 2:
        return

    ax.plot(historico, linewidth=2)
    ax.set_title("Histórico")
    ax.grid(True)
    canvas.draw()

# ================= CONTROLE DE TELA =================
def alternar_fullscreen(event=None):
    global fullscreen
    fullscreen = not fullscreen
    root.attributes("-fullscreen", fullscreen)


def sair_fullscreen(event=None):
    global fullscreen
    fullscreen = False
    root.attributes("-fullscreen", False)

# ================= INTERFACE =================
root = tk.Tk()
root.title("Estação Meteorológica")
root.geometry("900x600")
root.resizable(True, True)

root.bind("<F11>", alternar_fullscreen)
root.bind("<Escape>", sair_fullscreen)

# -------- Notebook (Abas) --------
notebook = ttk.Notebook(root)
notebook.pack(expand=True, fill="both")

# =================================================
# 🔹 ABA 1 – ESTAÇÃO METEOROLÓGICA
# =================================================
aba_sensores = ttk.Frame(notebook, padding=20)
notebook.add(aba_sensores, text="Sensores")

nome_label = ttk.Label(aba_sensores, text="--", font=("Arial", 22))
nome_label.pack(pady=10)

valor_label = ttk.Label(aba_sensores, text="--", font=("Arial", 36))
valor_label.pack(pady=10)

fig, ax = plt.subplots(figsize=(6, 2))
canvas = FigureCanvasTkAgg(fig, master=aba_sensores)
canvas.get_tk_widget().pack(pady=10, fill="x")

cfg = ttk.LabelFrame(aba_sensores, text="Configurações", padding=10)
cfg.pack(pady=10)

lim_min_var = tk.DoubleVar()
lim_max_var = tk.DoubleVar()
offset_var = tk.DoubleVar()

ttk.Label(cfg, text="Limite Mínimo").grid(row=0, column=0)
ttk.Entry(cfg, textvariable=lim_min_var).grid(row=0, column=1)

ttk.Label(cfg, text="Limite Máximo").grid(row=1, column=0)
ttk.Entry(cfg, textvariable=lim_max_var).grid(row=1, column=1)

ttk.Label(cfg, text="Offset").grid(row=2, column=0)
ttk.Entry(cfg, textvariable=offset_var).grid(row=2, column=1)

ttk.Button(cfg, text="Salvar", command=enviar_config).grid(
    row=3, column=0, columnspan=2, pady=5
)

btn_frame = ttk.Frame(aba_sensores)
btn_frame.pack(pady=10)

ttk.Button(btn_frame, text="Tela Cheia (F11)", command=alternar_fullscreen).grid(row=0, column=0, padx=5)
ttk.Button(btn_frame, text="Mudar Página", command=mudar_pagina).grid(row=0, column=1, padx=5)
ttk.Button(btn_frame, text="Sair", command=root.destroy).grid(row=0, column=2, padx=5)

# =================================================
# 🔹 ABA 2 – POTENCIÔMETROS (ADC)
# =================================================
aba_pots = ttk.Frame(notebook, padding=30)
notebook.add(aba_pots, text="Potenciômetros")

pot1_var = tk.StringVar(value="--")
pot2_var = tk.StringVar(value="--")

ttk.Label(aba_pots, text="Potenciômetro 1", font=("Arial", 16)).pack(pady=5)
pot1_bar = ttk.Progressbar(aba_pots, length=400, maximum=100)
pot1_bar.pack(pady=5)
ttk.Label(aba_pots, textvariable=pot1_var, font=("Arial", 14)).pack(pady=5)

ttk.Separator(aba_pots).pack(fill="x", pady=20)

ttk.Label(aba_pots, text="Potenciômetro 2", font=("Arial", 16)).pack(pady=5)
pot2_bar = ttk.Progressbar(aba_pots, length=400, maximum=100)
pot2_bar.pack(pady=5)
ttk.Label(aba_pots, textvariable=pot2_var, font=("Arial", 14)).pack(pady=5)

# ================= START =================
obter_estado()
root.mainloop()
