// Handing the player a file.

/**
 * Hand the browser a file to save.
 *
 * Built here and released again straight away: the blob is only needed for as
 * long as the click that reads it, and an object URL that is never revoked
 * holds its contents for the life of the document.
 */
export function download(name: string, text: string, type: string): void {
  const url = URL.createObjectURL(new Blob([text], { type }));
  const link = document.createElement("a");

  link.href = url;
  link.download = name;
  link.click();
  URL.revokeObjectURL(url);
}
