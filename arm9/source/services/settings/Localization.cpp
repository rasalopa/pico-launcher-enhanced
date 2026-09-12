#include "common.h"
#include <string.h>
#include "Localization.h"

static char sLanguage[16] = "english";

enum Language
{
    LANG_ENGLISH,
    LANG_SPANISH,
    LANG_FRENCH,
    LANG_GERMAN,
    LANG_ITALIAN,
    LANG_PORTUGUESE,
};

static Language currentLanguage()
{
    if (!strcasecmp(sLanguage, "spanish")) return LANG_SPANISH;
    if (!strcasecmp(sLanguage, "french")) return LANG_FRENCH;
    if (!strcasecmp(sLanguage, "german")) return LANG_GERMAN;
    if (!strcasecmp(sLanguage, "italian")) return LANG_ITALIAN;
    if (!strcasecmp(sLanguage, "portuguese")) return LANG_PORTUGUESE;
    return LANG_ENGLISH;
}

void Localization::SetLanguage(const char* language)
{
    if (!language || !*language)
    {
        strcpy(sLanguage, "english");
        return;
    }
    strncpy(sLanguage, language, sizeof(sLanguage) - 1);
    sLanguage[sizeof(sLanguage) - 1] = 0;
}

const char* Localization::GetLanguage() { return sLanguage; }

#define T(en, es, fr, de, it, pt) \
    switch (currentLanguage()) { \
        case LANG_SPANISH: return es; case LANG_FRENCH: return fr; case LANG_GERMAN: return de; \
        case LANG_ITALIAN: return it; case LANG_PORTUGUESE: return pt; default: return en; }

const char* Localization::DisplaySettings() { T("Display Settings", "Ajustes de pantalla", "Réglages d'affichage", "Anzeigeeinstellungen", "Impostazioni schermo", "Configurações de tela"); }
const char* Localization::Layout() { T("Layout", "Diseño", "Disposition", "Layout", "Layout", "Layout"); }
const char* Localization::Sorting() { T("Sorting", "Orden", "Tri", "Sortierung", "Ordinamento", "Ordenação"); }
const char* Localization::Light() { T("Light", "Brillo", "Luminosité", "Helligkeit", "Luminosità", "Brilho"); }
const char* Localization::Language() { T("Language", "Idioma", "Langue", "Sprache", "Lingua", "Idioma"); }
const char* Localization::Launcher() { T("Launcher", "Lanzador", "Lanceur", "Launcher", "Avvio", "Inicializador"); }
const char* Localization::PicoLauncher() { T("Pico", "Pico", "Pico", "Pico", "Pico", "Pico"); }
const char* Localization::BootstrapLauncher() { T("Bootstrap", "Bootstrap", "Bootstrap", "Bootstrap", "Bootstrap", "Bootstrap"); }
const char* Localization::Cheats() { T("Cheats", "Trucos", "Triches", "Cheats", "Trucchi", "Trapaças"); }
const char* Localization::Favorite() { T("Favorite", "Favorito", "Favori", "Favorit", "Preferito", "Favorito"); }
const char* Localization::RecentGames() { T("Recent games", "Juegos recientes", "Jeux récents", "Zuletzt gespielte Spiele", "Giochi recenti", "Jogos recentes"); }
const char* Localization::FavoriteGames() { T("Favorite games", "Juegos favoritos", "Jeux favoris", "Favoritenspiele", "Giochi preferiti", "Jogos favoritos"); }
const char* Localization::NothingPlayedYet() { T("Nothing played yet.", "Aún no has jugado nada.", "Aucun jeu joué pour le moment.", "Noch keine Spiele gespielt.", "Nessun gioco ancora giocato.", "Nenhum jogo jogado ainda."); }
const char* Localization::NoFavoritesYet() { T("No favorites yet. Press X on a game.", "Aún no hay favoritos. Pulsa X en un juego.", "Aucun favori. Appuyez sur X sur un jeu.", "Noch keine Favoriten. Drücke X bei einem Spiel.", "Nessun preferito. Premi X su un gioco.", "Ainda não há favoritos. Pressione X em um jogo."); }
const char* Localization::DeleteGame() { T("Delete game?", "¿Eliminar juego?", "Supprimer le jeu ?", "Spiel löschen?", "Eliminare il gioco?", "Excluir jogo?"); }
const char* Localization::DeleteHint() { T("X: delete    A/B: cancel", "X: eliminar    A/B: cancelar", "X : supprimer    A/B : annuler", "X: löschen    A/B: abbrechen", "X: elimina    A/B: annulla", "X: excluir    A/B: cancelar"); }
const char* Localization::SaveAlsoDeleted() { T("The save %s is also deleted", "La partida guardada %s también se eliminará", "La sauvegarde %s sera également supprimée", "Der Spielstand %s wird ebenfalls gelöscht", "Il salvataggio %s verrà eliminato", "O salvamento %s também será excluído"); }
const char* Localization::Statistics() { T("Statistics", "Estadísticas", "Statistiques", "Statistiken", "Statistiche", "Estatísticas"); }
const char* Localization::NoCheatsFound() { T("No cheats found.", "No se encontraron trucos.", "Aucune triche trouvée.", "Keine Cheats gefunden.", "Nessun trucco trovato.", "Nenhuma trapaça encontrada."); }
const char* Localization::CheatsAllOff() { T("X: all off", "X: desactivar todos", "X : tout désactiver", "X: alle aus", "X: disattiva tutto", "X: desativar todos"); }
const char* Localization::NoPreview() { T("No preview", "Sin vista previa", "Aucun aperçu", "Keine Vorschau", "Nessuna anteprima", "Sem pré-visualização"); }
const char* Localization::ScreenshotSaved() { T("Screenshot saved", "Captura guardada", "Capture d'écran enregistrée", "Screenshot gespeichert", "Screenshot salvato", "Captura de tela salva"); }
const char* Localization::ScreenshotSaveFailed() { T("Couldn't save the screenshot", "No se pudo guardar la captura", "Impossible d'enregistrer la capture d'écran", "Screenshot konnte nicht gespeichert werden", "Impossibile salvare lo screenshot", "Não foi possível salvar a captura de tela"); }
const char* Localization::ScreenshotStillSaving() { T("Still saving the last one", "Aún se está guardando la anterior", "La capture précédente est encore en cours d'enregistrement", "Der letzte Screenshot wird noch gespeichert", "Il precedente screenshot è ancora in salvataggio", "A captura anterior ainda está sendo salva"); }

const char* Localization::GameCountFormat() { T("%u %s", "%u %s", "%u %s", "%u %s", "%u %s", "%u %s"); }
const char* Localization::GameWord(unsigned count) { switch (currentLanguage()) { case LANG_SPANISH: return count == 1 ? "juego" : "juegos"; case LANG_FRENCH: return count == 1 ? "jeu" : "jeux"; case LANG_GERMAN: return count == 1 ? "Spiel" : "Spiele"; case LANG_ITALIAN: return count == 1 ? "gioco" : "giochi"; case LANG_PORTUGUESE: return count == 1 ? "jogo" : "jogos"; default: return count == 1 ? "game" : "games"; } }
const char* Localization::PlayedFavoritesCompletedFormat() { T("%u games played, %u favorites, %u completed", "%u juegos jugados, %u favoritos, %u completados", "%u jeux joués, %u favoris, %u terminés", "%u Spiele gespielt, %u Favoriten, %u abgeschlossen", "%u giochi giocati, %u preferiti, %u completati", "%u jogos jogados, %u favoritos, %u concluídos"); }
const char* Localization::PlayedFavoritesFormat() { T("%u games played, %u favorites", "%u juegos jugados, %u favoritos", "%u jeux joués, %u favoris", "%u Spiele gespielt, %u Favoriten", "%u giochi giocati, %u preferiti", "%u jogos jogados, %u favoritos"); }
const char* Localization::LaunchesPlayedFormat() { T("%u launches, %uh %02um played", "%u lanzamientos, %uh %02um de juego", "%u lancements, %uh %02um de jeu", "%u Starts, %uh %02um Spielzeit", "%u avvii, %uh %02um di gioco", "%u inicializações, %uh %02um de jogo"); }
const char* Localization::LaunchesTotalFormat() { T("%u launches in total", "%u lanzamientos en total", "%u lancements au total", "%u Starts insgesamt", "%u avvii in totale", "%u inicializações no total"); }
const char* Localization::LastPlayedFormat() { T("Last: %s (%c%c/%c%c %c%c:%c%c)", "Último: %s (%c%c/%c%c %c%c:%c%c)", "Dernier : %s (%c%c/%c%c %c%c:%c%c)", "Zuletzt: %s (%c%c/%c%c %c%c:%c%c)", "Ultimo: %s (%c%c/%c%c %c%c:%c%c)", "Último: %s (%c%c/%c%c %c%c:%c%c)"); }

const char* Localization::Month(unsigned month)
{
    static const char* const en[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    static const char* const es[] = { "Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic" };
    static const char* const fr[] = { "Jan", "Fév", "Mar", "Avr", "Mai", "Juin", "Juil", "Aoû", "Sep", "Oct", "Nov", "Déc" };
    static const char* const de[] = { "Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez" };
    static const char* const it[] = { "Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug", "Ago", "Set", "Ott", "Nov", "Dic" };
    static const char* const pt[] = { "Jan", "Fev", "Mar", "Abr", "Mai", "Jun", "Jul", "Ago", "Set", "Out", "Nov", "Dez" };
    if (month < 1 || month > 12) return "";
    const char* const* months = en;
    switch (currentLanguage()) { case LANG_SPANISH: months = es; break; case LANG_FRENCH: months = fr; break; case LANG_GERMAN: months = de; break; case LANG_ITALIAN: months = it; break; case LANG_PORTUGUESE: months = pt; break; default: break; }
    return months[month - 1];
}

#undef T
