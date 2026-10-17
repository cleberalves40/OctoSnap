<?xml version='1.0' encoding='utf-8'?>
<TS version="2.1" language="pt_BR">
<context>
    <name>FssBackend</name>
    <message>
        <location filename="../src/FssBackend.cpp" line="38" />
        <source>Could not start %1</source>
        <translation>Não foi possível iniciar %1</translation>
    </message>
    <message>
        <location filename="../src/FssBackend.cpp" line="48" />
        <source>Helper failed with code %1</source>
        <translation>O helper falhou com o código %1</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/MainWindow.cpp" line="50" />
        <source>OctoSnap — FFS Snapshots</source>
        <translation>OctoSnap — Snapshots FFS</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="58" />
        <source>OctoSnap started.</source>
        <translation>OctoSnap iniciado.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="67" />
        <source>Snapshot</source>
        <translation>Snapshot</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="67" />
        <location filename="../src/MainWindow.cpp" line="320" />
        <source>Active</source>
        <translation>Ativo</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="67" />
        <source>Mountpoint</source>
        <translation>Ponto de montagem</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="67" />
        <source>Space</source>
        <translation>Espaço</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="67" />
        <source>Created</source>
        <translation>Criado em</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="89" />
        <source>Snapshots use fss(4) with a copy-on-write backing store.

New snapshot: choose a mounted filesystem, then give the snapshot a name.
Mount read-only: mounts an active snapshot in an empty directory you choose.
Unmount: releases a read-only mount made by OctoSnap.
Restore: pick files or directories inside the snapshot and copy them back to
the live filesystem. A preview shows what changes; by default the current
version of each item is kept as &lt;item&gt;.octosnap-&lt;date&gt;. Directories are merged:
files created after the snapshot are kept.
Destroy selected: removes the snapshot. Destroying data cannot be undone.

Snapshots do not survive a reboot. Those marked Stale must be destroyed.</source>
        <translation>Snapshots usam fss(4) com armazenamento copy-on-write.

Novo snapshot: escolha um sistema de arquivos montado e defina um nome para o snapshot.
Montar somente leitura: monta um snapshot ativo em um diretório vazio à sua escolha.
Desmontar: libera a montagem somente leitura feita pelo OctoSnap.
Restaurar: selecione arquivos ou diretórios dentro do snapshot e copie-os de volta para
o sistema de arquivos ativo. Uma prévia exibe o que será alterado; por padrão a versão
atual de cada item é preservada como &lt;item&gt;.octosnap-&lt;data&gt;. Diretórios são mesclados:
arquivos criados após o snapshot são mantidos.
Destruir selecionado: remove o snapshot. A destruição dos dados não pode ser desfeita.

Snapshots não sobrevivem à reinicialização. Snapshots marcados como Obsoletos devem ser destruídos.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="101" />
        <source>Information</source>
        <translation>Informações</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="102" />
        <source>Output</source>
        <translation>Saída</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="103" />
        <source>Help</source>
        <translation>Ajuda</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="119" />
        <source>Actions</source>
        <translation>Ações</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="123" />
        <source>Refresh</source>
        <translation>Atualizar</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="129" />
        <source>Create</source>
        <translation>Criar</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="136" />
        <source>Mount read-only</source>
        <translation>Montar somente leitura</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="140" />
        <source>Unmount</source>
        <translation>Desmontar</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="144" />
        <location filename="../src/MainWindow.cpp" line="447" />
        <location filename="../src/MainWindow.cpp" line="467" />
        <location filename="../src/MainWindow.cpp" line="503" />
        <source>Restore</source>
        <translation>Restaurar</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="146" />
        <source>Copy files or directories from the snapshot back to the live filesystem</source>
        <translation>Copiar arquivos ou diretórios do snapshot de volta para o sistema de arquivos ativo</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="152" />
        <source>Destroy selected</source>
        <translation>Destruir selecionado</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="162" />
        <source>Filter by name…</source>
        <translation>Filtrar por nome…</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="170" />
        <source>&amp;File</source>
        <translation>&amp;Arquivo</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="173" />
        <source>&amp;Quit</source>
        <translation>&amp;Sair</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="175" />
        <source>&amp;Actions</source>
        <translation>&amp;Ações</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="184" />
        <source>&amp;Theme</source>
        <translation>&amp;Tema</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="197" />
        <source>Theme changed to %1.</source>
        <translation>Tema alterado para %1.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="201" />
        <source>&amp;Language</source>
        <translation>&amp;Idioma</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="206" />
        <source>System default</source>
        <translation>Padrão do sistema</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="219" />
        <source>Language</source>
        <translation>Idioma</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="220" />
        <source>Restart OctoSnap to apply the language change.</source>
        <translation>Reinicie o OctoSnap para aplicar a alteração de idioma.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="224" />
        <source>&amp;Help</source>
        <translation>A&amp;juda</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="225" />
        <source>&amp;About OctoSnap</source>
        <translation>&amp;Sobre o OctoSnap</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="236" />
        <source>Could not read snapshots: %1</source>
        <translation>Não foi possível ler os snapshots: %1</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/MainWindow.cpp" line="249" />
        <source>List updated (%n snapshot(s)).</source>
        <translation>
            <numerusform>Lista atualizada (%n snapshot).</numerusform>
            <numerusform>Lista atualizada (%n snapshots).</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="267" />
        <source>Stale</source>
        <translation>Obsoleto</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="268" />
        <source>RO</source>
        <translation>RO</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="308" />
        <source>Select a snapshot to see its details.</source>
        <translation>Selecione um snapshot para ver seus detalhes.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="314" />
        <source>Snapshot: %1</source>
        <translation>Snapshot: %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="315" />
        <source>Unit: %1</source>
        <translation>Unidade: %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="316" />
        <source>Filesystem: %1</source>
        <translation>Sistema de arquivos: %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="317" />
        <source>Backing store: %1</source>
        <translation>Armazenamento de apoio (backing store): %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="318" />
        <source>Space used: %1 (%2 bytes)</source>
        <translation>Espaço utilizado: %1 (%2 bytes)</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="319" />
        <source>Created: %1</source>
        <translation>Criado em: %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="320" />
        <source>State: %1</source>
        <translation>Estado: %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="320" />
        <source>Stale (not in fssconfig -l)</source>
        <translation>Obsoleto (não consta no fssconfig -l)</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="321" />
        <source>Read-only mount: %1</source>
        <translation>Montagem somente leitura: %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="321" />
        <source>none</source>
        <translation>nenhum</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/MainWindow.cpp" line="326" />
        <source>%n snapshot(s)</source>
        <translation>
            <numerusform>%n snapshot</numerusform>
            <numerusform>%n snapshots</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="330" />
        <source>Theme: %1</source>
        <translation>Tema: %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="341" />
        <source>No FFS filesystem is mounted.</source>
        <translation>Nenhum sistema de arquivos FFS está montado.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="353" />
        <location filename="../src/MainWindow.cpp" line="359" />
        <source>New snapshot</source>
        <translation>Novo snapshot</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="353" />
        <source>Filesystem:</source>
        <translation>Sistema de arquivos:</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="360" />
        <source>Snapshot name (letters, digits, '.', '_', '-'):</source>
        <translation>Nome do snapshot (letras, números, '.', '_', '-'):</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="366" />
        <source>Create snapshot</source>
        <translation>Criar snapshot</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="369" />
        <source>Snapshot '%1' created on %2.</source>
        <translation>Snapshot '%1' criado em %2.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="380" />
        <location filename="../src/MainWindow.cpp" line="395" />
        <source>Destroy snapshot</source>
        <translation>Destruir snapshot</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="381" />
        <source>Destroy snapshot "%1"?</source>
        <translation>Destruir o snapshot "%1"?</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="382" />
        <source>The live filesystem is not changed, but you lose the ability to recover data from this snapshot. This cannot be undone.</source>
        <translation>O sistema de arquivos ativo não é alterado, mas você perderá a capacidade de recuperar dados a partir deste snapshot. Esta ação não pode ser desfeita.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="386" />
        <source>Also delete the backing store file</source>
        <translation>Também excluir o arquivo de armazenamento de apoio (backing store)</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="398" />
        <source>Snapshot '%1' destroyed.</source>
        <translation>Snapshot '%1' destruído.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="404" />
        <source>Choose an empty directory for the read-only mount</source>
        <translation>Escolha um diretório vazio para a montagem somente leitura</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="410" />
        <source>Mount snapshot</source>
        <translation>Montar snapshot</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="413" />
        <source>Snapshot '%1' mounted read-only at %2.</source>
        <translation>Snapshot '%1' montado somente leitura em %2.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="432" />
        <source>Unmount snapshot</source>
        <translation>Desmontar snapshot</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="435" />
        <source>Unmounted %1.</source>
        <translation>Desmontado %1.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="448" />
        <source>Snapshot "%1" must be mounted read-only before its files can be restored. Mount it now?</source>
        <translation>O snapshot "%1" precisa estar montado em modo somente leitura antes que seus arquivos possam ser restaurados. Deseja montá-lo agora?</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/MainWindow.cpp" line="468" />
        <source>Overwrite %n item(s) on %1 with the version from "%2"?</source>
        <translation>
            <numerusform>Sobrescrever %n item em %1 com a versão de "%2"?</numerusform>
            <numerusform>Sobrescrever %n itens em %1 com a versão de "%2"?</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="470" />
        <source>The current contents of these items are replaced and cannot be recovered afterwards.</source>
        <translation>O conteúdo atual destes itens será substituído e não poderá ser recuperado posteriormente.</translation>
    </message>
    <message numerus="yes">
        <location filename="../src/MainWindow.cpp" line="477" />
        <source>Restoring %n item(s) from '%1'…</source>
        <translation>
            <numerusform>Restaurando %n item de '%1'…</numerusform>
            <numerusform>Restaurando %n itens de '%1'…</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="488" />
        <source>Restored %1</source>
        <translation>Restaurado %1</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="490" />
        <source>Current version of %1 kept as %2</source>
        <translation>Versão atual de %1 preservada como %2</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="492" />
        <source>Recreated missing directory %1</source>
        <translation>Diretório ausente %1 recriado</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="494" />
        <source>Skipped %1 (%2)</source>
        <translation>Ignorado %1 (%2)</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="496" />
        <source>Restore summary: %1 file(s), %2 director(ies), %3 symlink(s), %4 skipped, %5 error(s).</source>
        <translation>Resumo da restauração: %1 arquivo(s), %2 diretório(s), %3 link(s) simbólico(s), %4 ignorado(s), %5 erro(s).</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="506" />
        <source>Restore from '%1' finished.</source>
        <translation>Restauração a partir de '%1' concluída.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="507" />
        <source>Restore finished.</source>
        <translation>Restauração concluída.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="511" />
        <source>About OctoSnap</source>
        <translation>Sobre o OctoSnap</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="512" />
        <source>&lt;b&gt;OctoSnap %1&lt;/b&gt;&lt;br&gt;Snapshot manager for FFS/UFS on NetBSD, using fss(4).&lt;br&gt;Interface modeled after bectl and OctoPkgsrc.</source>
        <translation>&lt;b&gt;OctoSnap %1&lt;/b&gt;&lt;br&gt;Gerenciador de snapshots para FFS/UFS no NetBSD, utilizando fss(4).&lt;br&gt;Interface inspirada no bectl e OctoPkgsrc.</translation>
    </message>
    <message>
        <location filename="../src/MainWindow.cpp" line="519" />
        <source>Error: %1</source>
        <translation>Erro: %1</translation>
    </message>
</context>
<context>
    <name>RestoreDialog</name>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="31" />
        <source>missing</source>
        <translation>ausente</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="32" />
        <source>symlink → %1</source>
        <translation>link simbólico → %1</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="33" />
        <source>directory, %1</source>
        <translation>diretório, %1</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="43" />
        <source>Restore from snapshot "%1"</source>
        <translation>Restaurar a partir do snapshot "%1"</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="46" />
        <source>Select the files and directories to restore. They are copied from the snapshot (mounted at %1) back to the same place on %2.</source>
        <translation>Selecione os arquivos e diretórios que deseja restaurar. Eles serão copiados do snapshot (montado em %1) de volta para o mesmo local em %2.</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="70" />
        <source>Item</source>
        <translation>Item</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="70" />
        <source>In snapshot</source>
        <translation>No snapshot</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="70" />
        <source>Current version</source>
        <translation>Versão atual</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="70" />
        <source>Action</source>
        <translation>Ação</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="80" />
        <source>Keep the current version of each item as a backup (renamed to &lt;item&gt;.octosnap-&lt;date&gt;)</source>
        <translation>Manter a versão atual de cada item como backup (renomeado para &lt;item&gt;.octosnap-&lt;data&gt;)</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="86" />
        <source>Restore</source>
        <translation>Restaurar</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="134" />
        <source>Recreate</source>
        <translation>Recriar</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="136" />
        <source>Restore, keep current as backup</source>
        <translation>Restaurar, manter atual como backup</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="138" />
        <source>Merge into current directory</source>
        <translation>Mesclar no diretório atual</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="140" />
        <source>Conflict: file and directory</source>
        <translation>Conflito: arquivo e diretório</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="144" />
        <source>Overwrite (looks unchanged)</source>
        <translation>Sobrescrever (aparenta não ter alterações)</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="146" />
        <source>Overwrite</source>
        <translation>Sobrescrever</translation>
    </message>
    <message>
        <location filename="../src/RestoreDialog.cpp" line="151" />
        <source>Enable "keep the current version" to restore this item.</source>
        <translation>Habilite "manter a versão atual" para restaurar este item.</translation>
    </message>
</context>
<context>
    <name>Theme</name>
    <message>
        <location filename="../src/Theme.cpp" line="135" />
        <location filename="../src/Theme.cpp" line="141" />
        <source>System</source>
        <translation>Sistema</translation>
    </message>
    <message>
        <location filename="../src/Theme.cpp" line="136" />
        <source>Nord</source>
        <translation>Nord</translation>
    </message>
    <message>
        <location filename="../src/Theme.cpp" line="137" />
        <source>Catppuccin Frappé</source>
        <translation>Catppuccin Frappé</translation>
    </message>
    <message>
        <location filename="../src/Theme.cpp" line="138" />
        <source>Selenized Dark</source>
        <translation>Selenized Escuro</translation>
    </message>
    <message>
        <location filename="../src/Theme.cpp" line="139" />
        <source>Solarized Dark</source>
        <translation>Solarized Escuro</translation>
    </message>
</context>
</TS>