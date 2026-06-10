import streamlit as st
import subprocess
import re
import sys
import os
import pandas as pd
import plotly.graph_objects as go
from datetime import datetime, timedelta

# ----------------------------------------------------------------------
# Configuration – use the same binary name as compiled from C++ code
# Compile with: g++ -o scheduler scheduler.cpp  (or scheduler.exe on Windows)
# ----------------------------------------------------------------------
CPP_BINARY = "scheduler.exe" if sys.platform == "win32" else "./scheduler"

st.set_page_config(page_title="Task Scheduler", layout="wide")

st.markdown("""
<style>
@import url('https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=Syne:wght@600;800&display=swap');

html, body, [class*="css"] { font-family:'Syne',sans-serif; background:#09090b; color:#e4e4e7; }
h1,h2,h3 { font-family:'Syne',sans-serif; font-weight:800; }

div[data-testid="stSidebar"] { background:#0f0f11 !important; border-right:1px solid #27272a; }
div[data-testid="stSidebar"] label { color:#a1a1aa; font-size:0.78rem; letter-spacing:.05em; text-transform:uppercase; }

.stButton > button {
    background:#18181b; border:1px solid #3f3f46; color:#e4e4e7;
    border-radius:6px; font-family:'Space Mono',monospace; font-size:0.82rem; transition:all .15s;
}
.stButton > button:hover { border-color:#a78bfa; color:#a78bfa; background:#1c1624; }
.stButton > button[kind="primary"] { background:#6d28d9; border-color:#6d28d9; color:#fff; font-weight:700; }
.stButton > button[kind="primary"]:hover { background:#7c3aed; }

.task-card {
    background:#18181b; border:1px solid #27272a; border-radius:10px;
    padding:0.85rem 1.1rem; margin-bottom:0.55rem;
    display:flex; align-items:center; gap:1rem;
}
.task-name {
    font-family:'Syne',sans-serif; font-weight:700; font-size:1rem; color:#e4e4e7; flex:1;
}
.task-meta { font-family:'Space Mono',monospace; font-size:0.72rem; color:#71717a; }

.prio-badge {
    display:inline-block; border-radius:5px; padding:2px 10px;
    font-family:'Space Mono',monospace; font-size:0.68rem; font-weight:700; letter-spacing:.06em;
}
.prio-high   { background:#3b1219; color:#f87171; border:1px solid #7f1d1d; }
.prio-medium { background:#2d2008; color:#fbbf24; border:1px solid #78350f; }
.prio-low    { background:#0d2818; color:#4ade80; border:1px solid #14532d; }

.metric-card {
    background:#18181b; border:1px solid #27272a; border-radius:10px;
    padding:1rem 1.2rem; text-align:center;
}
.metric-card.best { border-color:#6d28d9; background:linear-gradient(135deg,#1a1030,#18181b); }
.metric-card h5 {
    font-family:'Space Mono',monospace; font-size:0.63rem; color:#71717a;
    margin:0 0 6px; letter-spacing:.1em; text-transform:uppercase;
}
.metric-card .val {
    font-family:'Space Mono',monospace; font-size:1.6rem; font-weight:700; color:#a78bfa; margin:0;
}
.metric-card .sub { font-size:0.68rem; color:#52525b; margin-top:4px; }

.best-badge {
    display:inline-block; background:#6d28d9; color:#fff;
    font-family:'Space Mono',monospace; font-size:0.65rem;
    padding:2px 8px; border-radius:4px; margin-left:6px;
}
.section-label {
    font-family:'Space Mono',monospace; font-size:0.7rem; color:#52525b;
    letter-spacing:.12em; text-transform:uppercase; margin-bottom:0.6rem;
}
.status-ok  { color:#4ade80; font-family:'Space Mono',monospace; font-size:0.8rem; }
.status-err { color:#f87171; font-family:'Space Mono',monospace; font-size:0.8rem; }

</style>
""", unsafe_allow_html=True)

# ----------------------------------------------------------------------
# Helper functions
# ----------------------------------------------------------------------
def prio_label(p):
    return {1: "High", 2: "Medium", 3: "Low"}.get(p, "?")

def prio_class(p):
    return {1: "prio-high", 2: "prio-medium", 3: "prio-low"}.get(p, "")

def fmt_dt(dt):
    return dt.strftime("%Y-%m-%d %H:%M:%S")

def fmt_duration(seconds):
    h, rem = divmod(int(seconds), 3600)
    m, s   = divmod(rem, 60)
    parts  = []
    if h: parts.append(f"{h}h")
    if m: parts.append(f"{m}m")
    if s or not parts: parts.append(f"{s}s")
    return " ".join(parts)

def today_dt(h, m):
    d = datetime.now().date()
    return datetime(d.year, d.month, d.day, int(h), int(m))

# ----------------------------------------------------------------------
# C++ binary communication
# ----------------------------------------------------------------------
def binary_exists():
    if os.path.isfile(CPP_BINARY):
        return True
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.isfile(os.path.join(script_dir, CPP_BINARY))

def run_cpp(commands):
    if not binary_exists():
        return "[ERROR] Binary not found. Compile with: g++ -o scheduler scheduler.cpp"
    stdin_text = "\n".join(commands + ["exit"]) + "\n"
    try:
        result = subprocess.run(
            [CPP_BINARY], input=stdin_text,
            capture_output=True, text=True, timeout=30,
        )
        return result.stdout
    except subprocess.TimeoutExpired:
        return "[ERROR] C++ process timed out."
    except Exception as e:
        return f"[ERROR] {e}"

def build_add_commands(tasks):
    """Build the exact sequence of commands expected by the C++ add routine."""
    cmds = ["add", str(len(tasks))]
    for t in tasks:
        cmds.append(t["name"])
        cmds.append(str(t["priority"]))
        cmds.append(fmt_dt(t["start_time"]))
        cmds.append(fmt_dt(t["end_time"]))
        if t.get("deadline"):
            cmds.append("y")
            cmds.append(fmt_dt(t["deadline"]))
        else:
            cmds.append("n")
    return cmds

# ----------------------------------------------------------------------
# Parse comparison output – robust against varying spaces
# ----------------------------------------------------------------------
def parse_compare_output(raw):
    # Pattern: algo, avg_wait, avg_tat, throughput, missed/total
    # Throughput is printed but not used in UI; we capture it but ignore.
    pattern = re.compile(
        r"^\s*(FCFS|Priority)\s+"
        r"([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+(\d+)/(\d+)",
        re.MULTILINE
    )
    all_rows = []
    for m in pattern.finditer(raw):
        all_rows.append({
            "algo":      m.group(1).strip(),
            "avg_wait":  float(m.group(2)),
            "avg_tat":   float(m.group(3)),
            "dl_missed": int(m.group(5)),
            "total":     int(m.group(6)),
        })
    rows = [r for r in all_rows if r["algo"] in {"FCFS", "Priority"}]
    best = min(rows, key=lambda r: r["avg_wait"]) if rows else None
    return rows, best

# ----------------------------------------------------------------------
# Execution order helpers (for UI preview only)
# ----------------------------------------------------------------------
def fcfs_order(tasks):
    return sorted(tasks, key=lambda t: t["start_time"])

def priority_order(tasks):
    return sorted(tasks, key=lambda t: (t["priority"], t["start_time"]))

# ----------------------------------------------------------------------
# Session state
# ----------------------------------------------------------------------
if "tasks" not in st.session_state:
    st.session_state.tasks = []

# ----------------------------------------------------------------------
# Sidebar – Add / Remove tasks
# ----------------------------------------------------------------------
with st.sidebar:
    st.markdown("## Task Scheduler")

    if binary_exists():
        st.markdown("<div class='status-ok'>✓ binary ready</div>", unsafe_allow_html=True)
    else:
        st.markdown("<div class='status-err'>✗ binary not found – compile first</div>",
                    unsafe_allow_html=True)
        st.code("g++ -o scheduler scheduler.cpp", language="bash")

    st.markdown("---")
    st.markdown("<div class='section-label'>Add Task</div>", unsafe_allow_html=True)

    name     = st.text_input("Task Name", placeholder="e.g. Data Backup")
    priority = st.selectbox("Priority", [1, 2, 3],
                            format_func=lambda x: prio_label(x))

    st.markdown("<div class='section-label' style='margin-top:.5rem'>Start Time</div>",
                unsafe_allow_html=True)
    c1, c2 = st.columns(2)
    with c1: sh = st.number_input("Hour", 0, 23, datetime.now().hour,   key="sh")
    with c2: sm = st.number_input("Min",  0, 59, datetime.now().minute, key="sm")

    st.markdown("<div class='section-label' style='margin-top:.5rem'>Duration</div>",
                unsafe_allow_html=True)
    d1, d2 = st.columns(2)
    with d1: dh = st.number_input("Hours", 0, 23, 0,  key="dh")
    with d2: dm = st.number_input("Mins",  0, 59, 30, key="dm")

    use_dl = st.checkbox("Set Deadline")
    dlh = dlm = 0
    if use_dl:
        st.markdown("<div class='section-label'>Deadline Time</div>", unsafe_allow_html=True)
        e1, e2 = st.columns(2)
        with e1: dlh = st.number_input("Hour", 0, 23, (datetime.now().hour + 2) % 24, key="dlh")
        with e2: dlm = st.number_input("Min",  0, 59, 0, key="dlm")

    if st.button("Add Task", use_container_width=True, type="primary"):
        dur_secs = dh * 3600 + dm * 60
        if not name.strip():
            st.error("Task name is required.")
        elif dur_secs == 0:
            st.error("Duration must be greater than 0.")
        elif any(t["name"] == name.strip() for t in st.session_state.tasks):
            st.error("A task with this name already exists.")
        else:
            start_dt = today_dt(sh, sm)
            end_dt   = start_dt + timedelta(seconds=dur_secs)
            dl_dt    = None
            if use_dl:
                dl_dt = today_dt(dlh, dlm)
                if dl_dt <= end_dt:
                    st.error("Deadline must be after end time.")
                    st.stop()
            st.session_state.tasks.append({
                "name":       name.strip(),
                "priority":   priority,
                "start_time": start_dt,
                "end_time":   end_dt,
                "duration":   dur_secs,
                "deadline":   dl_dt,
            })
            st.success(f"Added '{name.strip()}'")
            st.rerun()

    if st.session_state.tasks:
        st.markdown("---")
        st.markdown("<div class='section-label'>Remove Task</div>", unsafe_allow_html=True)
        del_name = st.selectbox("", [t["name"] for t in st.session_state.tasks],
                                label_visibility="collapsed")
        if st.button("Delete", use_container_width=True):
            st.session_state.tasks = [t for t in st.session_state.tasks
                                      if t["name"] != del_name]
            st.rerun()
        st.markdown("")
        if st.button("Clear All", use_container_width=True):
            st.session_state.tasks = []
            st.rerun()

# ----------------------------------------------------------------------
# Main area
# ----------------------------------------------------------------------
st.markdown("# Task Scheduler")
st.markdown("---")

tasks = st.session_state.tasks

if not tasks:
    st.markdown("""
    <div style='text-align:center;padding:5rem 0;color:#3f3f46'>
        <div style='font-size:3rem;margin-bottom:1rem'>📋</div>
        <div style='font-family:Space Mono;font-size:0.85rem'>
            No tasks yet — add one from the sidebar
        </div>
    </div>""", unsafe_allow_html=True)
    st.stop()

# ----------------------------------------------------------------------
# Display task cards
# ----------------------------------------------------------------------
st.markdown("<div class='section-label'>Your Tasks</div>", unsafe_allow_html=True)

for t in sorted(tasks, key=lambda x: x["priority"]):
    dl_text = (
        f"&nbsp;&nbsp;&nbsp;Deadline: <b>{t['deadline'].strftime('%H:%M')}</b>"
        if t.get("deadline") else ""
    )
    st.markdown(f"""
    <div class="task-card">
        <div>
            <span class="prio-badge {prio_class(t['priority'])}">{prio_label(t['priority'])}</span>
        </div>
        <div class="task-name">{t['name']}</div>
        <div class="task-meta">
            {t['start_time'].strftime('%H:%M')} &rarr; {t['end_time'].strftime('%H:%M')}
            &nbsp;&nbsp;&nbsp;Duration: {fmt_duration(t['duration'])}
            {dl_text}
        </div>
    </div>""", unsafe_allow_html=True)

# ----------------------------------------------------------------------
# Execution order preview
# ----------------------------------------------------------------------
st.markdown("")
st.markdown("<div class='section-label'>Execution Order Preview</div>", unsafe_allow_html=True)

tab1, tab2 = st.tabs(["FCFS Order", "Priority Order"])

with tab1:
    st.caption("Tasks will execute in arrival order (earliest start time first)")
    for i, t in enumerate(fcfs_order(tasks), 1):
        dl_text = f"  |  Deadline: {t['deadline'].strftime('%H:%M')}" if t.get("deadline") else ""
        st.markdown(f"""
        <div class="task-card" style="border-left:3px solid #60a5fa;">
            <div style="font-family:Space Mono;font-size:1rem;color:#60a5fa;font-weight:700;min-width:28px">#{i}</div>
            <div><span class="prio-badge {prio_class(t['priority'])}">{prio_label(t['priority'])}</span></div>
            <div class="task-name">{t['name']}</div>
            <div class="task-meta">{t['start_time'].strftime('%H:%M')} &rarr; {t['end_time'].strftime('%H:%M')} &nbsp; {fmt_duration(t['duration'])}{dl_text}</div>
        </div>""", unsafe_allow_html=True)

with tab2:
    st.caption("Tasks will execute by priority — High first, then Medium, then Low")
    for i, t in enumerate(priority_order(tasks), 1):
        dl_text = f"  |  Deadline: {t['deadline'].strftime('%H:%M')}" if t.get("deadline") else ""
        st.markdown(f"""
        <div class="task-card" style="border-left:3px solid #fbbf24;">
            <div style="font-family:Space Mono;font-size:1rem;color:#fbbf24;font-weight:700;min-width:28px">#{i}</div>
            <div><span class="prio-badge {prio_class(t['priority'])}">{prio_label(t['priority'])}</span></div>
            <div class="task-name">{t['name']}</div>
            <div class="task-meta">{t['start_time'].strftime('%H:%M')} &rarr; {t['end_time'].strftime('%H:%M')} &nbsp; {fmt_duration(t['duration'])}{dl_text}</div>
        </div>""", unsafe_allow_html=True)

st.markdown("")

# ----------------------------------------------------------------------
# Run comparison
# ----------------------------------------------------------------------
if st.button("Run and Compare Algorithms", use_container_width=True, type="primary"):
    with st.spinner("Running C++ scheduler..."):
        cmds   = build_add_commands(tasks) + ["compare"]
        output = run_cpp(cmds)
    st.session_state["last_output"] = output

# ----------------------------------------------------------------------
# Display results
# ----------------------------------------------------------------------
if "last_output" in st.session_state:
    raw = st.session_state["last_output"]
    rows, best = parse_compare_output(raw)

    st.markdown("---")

    if rows and best:
        st.markdown("## Results")
        st.markdown(
            f"Best algorithm for this task set: "
            f"<span class='best-badge'>Best: {best['algo']}</span>",
            unsafe_allow_html=True,
        )
        st.markdown("")

        # Metric cards
        cols = st.columns(len(rows))
        for col, r in zip(cols, rows):
            is_best  = r["algo"] == best["algo"]
            card_cls = "metric-card best" if is_best else "metric-card"
            with col:
                st.markdown(f"""
                <div class="{card_cls}">
                    <h5>{r['algo']}</h5>
                    <p class="val">{fmt_duration(int(r['avg_wait']))}</p>
                    <div class="sub">average waiting time</div>
                    <div class="sub">{r['dl_missed']} of {r['total']} deadlines missed</div>
                </div>""", unsafe_allow_html=True)

        st.markdown("")
        st.markdown("---")
        st.markdown("<div class='section-label'>Explanation</div>", unsafe_allow_html=True)
        fcfs_wait = next(r["avg_wait"] for r in rows if r["algo"] == "FCFS")
        pri_wait  = next(r["avg_wait"] for r in rows if r["algo"] == "Priority")
        diff      = round(abs(fcfs_wait - pri_wait), 1)

        if pri_wait < fcfs_wait:
            st.info(
                f"Priority Scheduling made tasks wait {fmt_duration(int(diff))} less on average "
                f"by running high-priority tasks first instead of arrival order."
            )
        elif fcfs_wait < pri_wait:
            st.info(
                f"FCFS performed better by {fmt_duration(int(diff))} for this task set — "
                f"tasks arrived in a good order so no reordering was needed."
            )
        else:
            st.info("Both algorithms performed equally for this task set.")
    else:
        st.warning("Could not read results from C++ output. See raw output below.")
        with st.expander("Raw C++ output"):
            st.code(raw)