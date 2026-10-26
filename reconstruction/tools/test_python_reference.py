"""Verifier le patch de correction sans modifier le Python historique.

Copie de travail sous reconstruction/build, sans modification de l'index
Git. Cette reference de test reste un outil d'audit, pas la verite originale.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys

WORK = Path(__file__).resolve().parents[1]
ROOT = WORK.parent


def prepare_reference_copy() -> Path:
    """Recopier les sources historiques et le JSON dans le dossier de test.

    Repartir des sources a chaque execution evite de reappliquer le patch
    sur du code deja corrige. La copie n'emporte pas les caches Python.
    Retourne le dossier cree ; les originaux sont uniquement lus.
    """
    stage = WORK / "build" / "python_reference"
    stage.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT / "src", stage / "src", dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
    (stage / "data").mkdir(exist_ok=True)
    shutil.copyfile(ROOT / "data/game_data.json", stage / "data/game_data.json")
    return stage


def apply_corrections_to_copy(stage: Path) -> None:
    """Appliquer le patch archive uniquement a la copie de reference.

    --directory prefixe les chemins src/ du patch avec le dossier de test.
    Sans --index ni --cached, git apply ne prepare aucun fichier pour commit.
    En cas d'incompatibilite avec la source, arreter avant de lancer pytest.
    """
    patch = WORK / "patches/python-combat-corrections.patch"
    prefix = stage.relative_to(ROOT).as_posix()
    subprocess.run(["git", "apply", f"--directory={prefix}", str(patch)],
                   cwd=ROOT, check=True)


def run_provenance_tests(stage: Path) -> None:
    """Executer les vecteurs de source en important la copie corrigee.

    PYTHONPATH dirige les imports vers stage/src ; le marqueur d'audit
    autorise ces tests, normalement exclus de la suite historique.
    L'environnement est local au sous-processus, et les caches restent
    sous reconstruction/build. Une regression fait echouer le script.
    """
    env = os.environ.copy()
    env["PYTHONPATH"] = str(stage / "src")
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    env["TERA_RECONSTRUCTION_PYTHON_AUDIT"] = "1"
    subprocess.run([sys.executable, "-m", "pytest", "-q", "-o",
                    f"cache_dir={WORK / 'build/pytest_cache'}",
                    str(WORK / "tests/test_combat_provenance.py")],
                   cwd=stage, env=env, check=True)


def main() -> None:
    """Reconstruire la copie, appliquer les corrections, puis verifier le resultat."""
    stage = prepare_reference_copy()
    apply_corrections_to_copy(stage)
    run_provenance_tests(stage)


if __name__ == "__main__":
    main()
